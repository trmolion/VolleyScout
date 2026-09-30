#include "historypanel.h"
#include "setdelegate.h"

#include "volleyscout/commandlistmodel.h"
#include "volleyscout/entrymodel.h"
#include "volleyscout/entryproxymodel.h"
#include "volleyscout/rostermodel.h"
#include "volleyscout/textformat.h"

#include "theme/icons.h"
#include "theme/thememanager.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QStackedWidget>
#include <QStyledItemDelegate>
#include <QCoreApplication>
#include <QStyle>
#include <QTableView>
#include <QVBoxLayout>

namespace {

constexpr int kAllSets = 0; // id кнопки фильтра «Все»
enum Page { HistoryPage, TeamPage }; // вкладки правой панели в режиме «Таблица»

const char *const kSyntaxHelp =
    "13s+ — номер · действие · оценка\n"
    "Действия: s подача, r приём, a атака, b блок, d защита\n"
    "Оценки: # отлично, + хорошо, ! средне (только r, d), - плохо, = ошибка";

// Свойство «validation» (none / ok / warn / error) — по нему BeachStyle красит рамку поля и подсказку.
void setValidation(QWidget *widget, const char *state)
{
    if (widget->property("validation").toByteArray() == state)
        return;
    widget->setProperty("validation", QByteArray(state));
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

// Похоже на команду по форме, но отвергнуто правилом («!» у s/a/b) — это ошибка;
// иначе ввод просто ещё не завершён — предупреждение.
const char *invalidState(const QString &text)
{
    static const QRegularExpression shape(QStringLiteral("^\\d{1,2}[srabd][#+!\\-=]$"));
    return shape.match(text).hasMatch() ? "error" : "warn";
}

// ФИО ещё не введено — блеклая подсказка на месте имени (в модели остаётся пустая строка).
class RosterNameDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override
    {
        QStyledItemDelegate::initStyleOption(option, index);
        if (!option->text.isEmpty())
            return;
        option->text = QCoreApplication::translate("HistoryPanel", "Введите ФИО игрока");
        const QColor placeholder = ThemeManager::tokens().color.placeholder;
        option->palette.setColor(QPalette::Text, placeholder);
        option->palette.setColor(QPalette::HighlightedText, placeholder);
    }
};

} // namespace

HistoryPanel::HistoryPanel(vs::EntryModel *entries, vs::CommandListModel *commands, vs::RosterModel *roster,
                           QWidget *parent)
    : QWidget(parent)
    , m_entries(entries)
    , m_commands(commands)
    , m_roster(roster)
    , m_entriesProxy(new vs::EntryProxyModel(this))
    , m_rowDelegate(new SetSeparatorDelegate(this))
    , m_setDelegate(new SetDelegate(this))
    , m_title(new QWidget(this))
    , m_tabBar(new QWidget(this))
    , m_tabGroup(new QButtonGroup(this))
    , m_pages(new QStackedWidget(this))
    , m_rosterView(new QTableView(this))
    , m_countLabel(new QLabel(this))
    , m_filterBar(new QWidget(this))
    , m_filterGroup(new QButtonGroup(this))
    , m_view(new QTableView(this))
    , m_inputArea(new QWidget(this))
    , m_setGroup(new QButtonGroup(this))
    , m_setButtons(new QWidget(this))
    , m_timeLabel(new QLabel(this))
    , m_input(new QLineEdit(this))
    , m_submitButton(new QPushButton(Icons::get(QStringLiteral("enter")), QString(), this))
    , m_hintLabel(new QLabel(this))
{
    m_entriesProxy->setSourceModel(m_entries);

    // --- Заголовок: «Формирование» ---
    using namespace tokens;
    setProperty("surface", "panel2");
    setProperty("borders", "left");

    auto *titleLabel = new QLabel(tr("ИСТОРИЯ"), m_title);
    titleLabel->setProperty("role", "sectionLabel");
    m_countLabel->setProperty("role", "muted");
    auto *title = new QHBoxLayout(m_title);
    title->setContentsMargins(spacing::xl, spacing::l, spacing::xl, spacing::l);
    title->addWidget(titleLabel);
    title->addStretch();
    title->addWidget(m_countLabel);

    // --- Заголовок: «Таблица» — две вкладки равной ширины ---
    auto *tabSegment = new QWidget(m_tabBar);
    tabSegment->setProperty("segmented", "tabs");
    auto *tabBarLayout = new QHBoxLayout(m_tabBar);
    tabBarLayout->setContentsMargins(spacing::l, spacing::l, spacing::l, spacing::s);
    tabBarLayout->addWidget(tabSegment);
    auto *tabs = new QHBoxLayout(tabSegment);
    for (auto [page, text] : {std::pair{HistoryPage, tr("История")}, std::pair{TeamPage, tr("Команда")}}) {
        auto *button = new QPushButton(text, tabSegment);
        button->setCheckable(true);
        button->setFocusPolicy(Qt::NoFocus);
        m_tabGroup->addButton(button, page);
        tabs->addWidget(button, 1);
    }
    m_tabGroup->button(HistoryPage)->setChecked(true);

    // --- Фильтр по партиям ---
    m_filterBar->setProperty("segmented", "chips");
    auto *filterLayout = new QHBoxLayout(m_filterBar);
    filterLayout->setContentsMargins(spacing::l, 0, spacing::l, spacing::m);
    for (int set = kAllSets; set <= vs::kMaxSets; ++set) {
        auto *button = new QPushButton(set == kAllSets ? tr("Все") : QStringLiteral("s%1").arg(set), m_filterBar);
        button->setCheckable(true);
        button->setFocusPolicy(Qt::NoFocus);
        m_filterGroup->addButton(button, set);
        filterLayout->addWidget(button);
    }
    filterLayout->addStretch();
    m_filterGroup->button(kAllSets)->setChecked(true);

    // --- История ---
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->verticalHeader()->hide();
    m_view->setWordWrap(false);

    // --- Команда: номер → ФИО ---
    m_rosterView->setModel(m_roster);
    m_rosterView->setItemDelegateForColumn(vs::RosterModel::NameColumn, new RosterNameDelegate(m_rosterView));
    m_rosterView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_rosterView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_rosterView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked
                                  | QAbstractItemView::EditKeyPressed | QAbstractItemView::AnyKeyPressed);
    m_rosterView->verticalHeader()->hide();
    m_rosterView->horizontalHeader()->setSectionResizeMode(vs::RosterModel::NumberColumn,
                                                           QHeaderView::ResizeToContents);
    m_rosterView->horizontalHeader()->setSectionResizeMode(vs::RosterModel::NameColumn, QHeaderView::Stretch);

    auto *historyPage = new QWidget(m_pages);
    auto *historyLayout = new QVBoxLayout(historyPage);
    historyLayout->setContentsMargins(0, 0, 0, 0);
    historyLayout->addWidget(m_filterBar);
    historyLayout->addWidget(m_view, 1);

    auto *teamPage = new QWidget(m_pages);
    auto *teamLayout = new QVBoxLayout(teamPage);
    teamLayout->setContentsMargins(0, 0, 0, 0);
    teamLayout->addWidget(m_rosterView, 1);

    m_pages->insertWidget(HistoryPage, historyPage);
    m_pages->insertWidget(TeamPage, teamPage);

    // --- Ввод команды ---
    m_setButtons->setProperty("segmented", "sets");
    auto *setsTitle = new QLabel(tr("ПАРТИЯ"), m_setButtons);
    setsTitle->setProperty("role", "sectionLabel");
    auto *setLayout = new QHBoxLayout(m_setButtons);
    setLayout->setContentsMargins(0, 0, 0, 0);
    setLayout->addWidget(setsTitle);
    setLayout->addStretch();
    for (int set = 1; set <= vs::kMaxSets; ++set) {
        auto *button = new QPushButton(QStringLiteral("s%1").arg(set), m_setButtons);
        button->setCheckable(true);
        button->setFocusPolicy(Qt::NoFocus);
        m_setGroup->addButton(button, set);
        setLayout->addWidget(button);
    }
    m_setGroup->button(1)->setChecked(true);

    m_input->setPlaceholderText(QStringLiteral("13s+"));
    m_input->setMaxLength(8);
    m_input->setClearButtonEnabled(true);
    m_input->setToolTip(QString::fromUtf8(kSyntaxHelp));
    m_submitButton->setToolTip(tr("Записать команду"));
    m_hintLabel->setWordWrap(true);
    m_hintLabel->setProperty("validation", "none");
    m_hintLabel->setMinimumHeight(m_hintLabel->fontMetrics().height()); // строка проверки не сдвигает панель
    m_input->setProperty("size", "input");
    m_input->setProperty("validation", "none");
    m_submitButton->setProperty("variant", "primary");
    m_submitButton->setProperty("size", "input");
    m_timeLabel->setProperty("role", "timecode");

    auto *inputRow = new QHBoxLayout;
    inputRow->addWidget(m_timeLabel);
    inputRow->addWidget(m_input, 1);
    inputRow->addWidget(m_submitButton);

    m_inputArea->setProperty("surface", "panel");
    m_inputArea->setProperty("borders", "top");
    auto *inputLayout = new QVBoxLayout(m_inputArea);
    inputLayout->setContentsMargins(spacing::xl, 14, spacing::xl, spacing::xl);
    inputLayout->setSpacing(10);
    inputLayout->addWidget(m_setButtons);
    inputLayout->addLayout(inputRow);
    inputLayout->addWidget(m_hintLabel);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_title);
    layout->addWidget(m_tabBar);
    layout->addWidget(m_pages, 1);
    layout->addWidget(m_inputArea);

    connect(m_filterGroup, &QButtonGroup::idClicked, this, [this](int set) {
        m_entriesProxy->setSetFilter(set);
    });
    connect(m_setGroup, &QButtonGroup::idClicked, this, [this] {
        updateHint();
        focusInput();
    });
    connect(m_view, &QTableView::clicked, this, &HistoryPanel::onViewClicked);
    connect(m_tabGroup, &QButtonGroup::idClicked, m_pages, &QStackedWidget::setCurrentIndex);
    // Строка состава — подсветить этого игрока в таблице слева.
    connect(m_rosterView, &QTableView::clicked, this, [this](const QModelIndex &index) {
        m_pickedPlayer = index.data(vs::RosterModel::PlayerRole).toInt();
        emit playerPicked(m_pickedPlayer);
    });
    connect(m_input, &QLineEdit::textChanged, this, &HistoryPanel::updateHint);
    connect(m_input, &QLineEdit::returnPressed, this, &HistoryPanel::submit);
    connect(m_submitButton, &QPushButton::clicked, this, &HistoryPanel::submit);

    for (QAbstractItemModel *model : {static_cast<QAbstractItemModel *>(m_entries),
                                      static_cast<QAbstractItemModel *>(m_commands)}) {
        connect(model, &QAbstractItemModel::rowsInserted, this, &HistoryPanel::updateCounters);
        connect(model, &QAbstractItemModel::rowsRemoved, this, &HistoryPanel::updateCounters);
        connect(model, &QAbstractItemModel::modelReset, this, &HistoryPanel::updateCounters);
        connect(model, &QAbstractItemModel::dataChanged, this, &HistoryPanel::updateCounters);
    }
    connect(m_entries, &vs::EntryModel::trimChanged, this, &HistoryPanel::updateCounters);

    // Новая запись — прокрутить к ней.
    connect(m_entries, &QAbstractItemModel::rowsInserted, this, [this](const QModelIndex &, int first) {
        if (m_mode == Mode::Formation)
            m_view->scrollTo(m_entriesProxy->mapFromSource(m_entries->index(first, 0)));
    });

    setCurrentTime(0);
    setMode(Mode::Formation);
}

void HistoryPanel::setMode(Mode mode)
{
    m_mode = mode;
    const bool formation = mode == Mode::Formation;

    m_view->setModel(formation ? static_cast<QAbstractItemModel *>(m_entriesProxy)
                               : static_cast<QAbstractItemModel *>(m_commands));
    // Разделитель партий нужен только истории «Формирования»: в «Таблице» одна партия.
    // Строки в обоих режимах рисует один делегат; разделитель партий в «Таблице» не появляется —
    // у действий скаута нет номера партии.
    m_view->setItemDelegate(m_rowDelegate);
    m_view->setItemDelegateForColumn(vs::EntryModel::SetColumn, formation ? m_setDelegate : nullptr);
    m_view->setEditTriggers(formation ? QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed
                                      : QAbstractItemView::EditTriggers(QAbstractItemView::NoEditTriggers));

    QHeaderView *header = m_view->horizontalHeader();
    header->setStretchLastSection(false);
    for (int column = 0; column < m_view->model()->columnCount(); ++column)
        header->setSectionResizeMode(column, QHeaderView::ResizeToContents);
    // В узкой панели «Формирования» расшифровка — во всплывающей подсказке к команде.
    for (int column = 0; column < m_view->model()->columnCount(); ++column)
        m_view->setColumnHidden(column, formation && column == vs::EntryModel::DescriptionColumn);
    header->setSectionResizeMode(formation ? int(vs::EntryModel::CommandColumn)
                                           : int(vs::CommandListModel::DescriptionColumn),
                                 QHeaderView::Stretch);

    m_title->setVisible(formation);
    m_tabBar->setVisible(!formation);
    m_pages->setCurrentIndex(formation ? HistoryPage : m_tabGroup->checkedId());
    m_filterBar->setVisible(formation);
    // В «Таблице» ввода нет — блок скрыт целиком; setEnabled — чтобы пробел и Enter его не задели.
    m_inputArea->setVisible(formation);
    m_input->setEnabled(formation);
    if (!formation)
        m_input->clear();

    m_pickedPlayer = -1;
    updateCounters();
    updateHint();
}

int HistoryPanel::currentSet() const
{
    return m_setGroup->checkedId();
}

QWidget *HistoryPanel::commandInput() const
{
    return m_input;
}

void HistoryPanel::setCurrentTime(qint64 trimmedMs)
{
    m_timeLabel->setText(vs::formatTime(trimmedMs));
}

void HistoryPanel::focusInput()
{
    if (m_input->isEnabled())
        m_input->setFocus();
}

void HistoryPanel::submit()
{
    if (m_mode != Mode::Formation)
        return;

    const vs::ParseResult parsed = vs::parseCommand(m_input->text());
    if (!parsed.ok()) {
        updateHint();
        return;
    }
    emit commandSubmitted(*parsed.command, currentSet());
    m_input->clear();
}

void HistoryPanel::updateHint()
{
    if (m_mode != Mode::Formation) {
        m_hintLabel->clear();
        setValidation(m_input, "none");
        return;
    }

    const QString text = m_input->text().trimmed();
    if (text.isEmpty()) {
        m_hintLabel->clear();
        setValidation(m_input, "none");
        setValidation(m_hintLabel, "none");
        return;
    }

    const vs::ParseResult parsed = vs::parseCommand(text);
    const char *state = parsed.ok() ? "ok" : invalidState(text);
    m_hintLabel->setText(parsed.ok() ? QStringLiteral("✓ ") + parsed.command->describe() : parsed.error);
    setValidation(m_input, state);
    setValidation(m_hintLabel, state);
}

void HistoryPanel::updateCounters()
{
    if (m_mode == Mode::Table)
        return;

    const QList<vs::ScoutEntry> &entries = m_entries->entries();
    QString count = QString::number(entries.size());
    if (const int outside = m_entries->outOfRangeCount())
        count += tr(" · %1 вне").arg(outside);
    m_countLabel->setText(count);
    m_countLabel->setToolTip(vs::pluralRu(entries.size(), tr("запись"), tr("записи"), tr("записей")));

    for (int set = kAllSets; set <= vs::kMaxSets; ++set) {
        const auto n = set == kAllSets
                           ? entries.size()
                           : std::count_if(entries.cbegin(), entries.cend(),
                                           [set](const vs::ScoutEntry &e) { return e.set == set; });
        const QString title = set == kAllSets ? tr("Все партии") : tr("Партия %1").arg(set);
        m_filterGroup->button(set)->setToolTip(
            QStringLiteral("%1: %2").arg(title, vs::pluralRu(n, tr("запись"), tr("записи"), tr("записей"))));
    }
}

void HistoryPanel::onViewClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    if (m_mode == Mode::Table) {
        const int player = index.data(vs::CommandListModel::PlayerRole).toInt();
        m_pickedPlayer = m_pickedPlayer == player ? -1 : player;
        if (m_pickedPlayer < 0)
            m_view->clearSelection();
        emit playerPicked(m_pickedPlayer);
        return;
    }

    const QModelIndex source = m_entriesProxy->mapToSource(index);
    switch (source.column()) {
    case vs::EntryModel::TimeColumn:
        if (source.data(vs::EntryModel::InRangeRole).toBool())
            emit seekRequested(source.data(vs::EntryModel::TrimmedTimeRole).toLongLong());
        break;
    case vs::EntryModel::RemoveColumn:
        m_entries->removeEntry(source.row());
        break;
    default:
        break;
    }
}
