#include "stylegallery.h"
#include "setdelegate.h"
#include "sheetdelegate.h"
#include "timelinewidget.h"
#include "theme/fonts.h"
#include "theme/icons.h"
#include "theme/thememanager.h"

#include "volleyscout/entrymodel.h"
#include "volleyscout/entryproxymodel.h"
#include "volleyscout/statstablemodel.h"

#include <QButtonGroup>
#include <QComboBox>
#include <QDateEdit>
#include <QDirIterator>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QScrollBar>
#include <QTableView>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

using namespace tokens;

// --- Образцы, которые рисуются по токенам в момент отрисовки (переживают смену темы) ---

class SwatchGrid : public QWidget
{
public:
    using QWidget::QWidget;
    QSize minimumSizeHint() const override { return sizeHint(); }
    QSize sizeHint() const override { return {1300, 150}; }

protected:
    void paintEvent(QPaintEvent *) override
    {
        const ThemeTokens::Colors &c = ThemeManager::tokens().color;
        const std::pair<const char *, QColor> swatches[] = {
            {"bg", c.bg}, {"panel", c.panel}, {"panel2", c.panel2}, {"border", c.border},
            {"rowDivider", c.rowDivider}, {"text", c.text}, {"textMuted", c.textMuted}, {"accent", c.accent},
            {"onAccent", c.onAccent}, {"accentSoft", c.accentSoft}, {"link", c.link}, {"inputBg", c.inputBg},
            {"video", c.video}, {"trimCut", c.trimCut}, {"danger", c.danger}, {"placeholder", c.placeholder},
        };
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        int i = 0;
        for (const auto &[name, color] : swatches) {
            const QRectF box((i % 12) * 98.0, (i / 12) * 76.0, 86, 34);
            p.setPen(QPen(c.border, 1));
            p.setBrush(color);
            p.drawRoundedRect(box.adjusted(0.5, 0.5, -0.5, -0.5), 5, 5);
            p.setPen(c.text);
            p.setFont(Fonts::ui(font::caption));
            p.drawText(QPointF(box.left(), box.bottom() + 14), QLatin1String(name));
            p.setPen(c.textMuted);
            p.setFont(Fonts::mono(font::sheetSmall));
            p.drawText(QPointF(box.left(), box.bottom() + 27), color.name().toUpper());
            ++i;
        }
    }
};

class GradeRow : public QWidget
{
public:
    using QWidget::QWidget;
    QSize minimumSizeHint() const override { return sizeHint(); }
    QSize sizeHint() const override { return {300, 64}; }

protected:
    void paintEvent(QPaintEvent *) override
    {
        const ThemeTokens &t = ThemeManager::tokens();
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        int x = 0;
        for (vs::Grade grade : vs::kAllGrades) {
            const QRectF box(x, 0, 44, 44);
            p.setPen(QPen(t.color.border, 1));
            p.setBrush(t.color.panel);
            p.drawRoundedRect(box.adjusted(0.5, 0.5, -0.5, -0.5), radius::control, radius::control);
            p.setPen(t.gradeColor(grade));
            p.setFont(Fonts::mono(22, 600));
            p.drawText(box, Qt::AlignCenter, QString(vs::gradeCode(grade)));
            p.setPen(t.color.textMuted);
            p.setFont(Fonts::mono(font::sheetSmall));
            p.drawText(QRectF(x - 6, 46, 56, 16), Qt::AlignCenter, t.gradeColor(grade).name().toUpper());
            x += 62;
        }
    }
};

class IconGrid : public QWidget
{
public:
    explicit IconGrid(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        QDirIterator it(QStringLiteral(":/icons"), {QStringLiteral("*.svg")});
        while (it.hasNext())
            m_names << QFileInfo(it.next()).completeBaseName();
        m_names.sort();
    }
    QSize sizeHint() const override { return {1300, int((m_names.size() + 11) / 12) * 80}; }
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent(QPaintEvent *) override
    {
        const ThemeTokens::Colors &c = ThemeManager::tokens().color;
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        for (int i = 0; i < m_names.size(); ++i) {
            const int x = (i % 12) * 104 + 24;
            const int y = (i / 12) * 80;
            const QRectF box(x, y, 44, 44);
            p.setPen(QPen(c.border, 1));
            p.setBrush(c.panel);
            p.drawRoundedRect(box.adjusted(0.5, 0.5, -0.5, -0.5), radius::input, radius::input);
            p.drawPixmap(QRect(x + 12, y + 12, 20, 20), Icons::pixmap(m_names[i], 20, c.text, devicePixelRatioF()));
            p.setPen(c.textMuted);
            p.setFont(Fonts::mono(font::sheetSmall));
            p.drawText(QRectF(x - 30, y + 48, 104, 16), Qt::AlignCenter, m_names[i]);
        }
    }

private:
    QStringList m_names;
};

class TooltipSample : public QWidget
{
public:
    using QWidget::QWidget;
    QSize minimumSizeHint() const override { return sizeHint(); }
    QSize sizeHint() const override { return {200, 30}; }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        QStyleOption option;
        option.initFrom(this);
        const QString text = QStringLiteral("№13 · подача · хорошо");
        p.setFont(Fonts::ui(font::small));
        option.rect = QRect(0, 0, p.fontMetrics().horizontalAdvance(text) + 20, 28);
        style()->drawPrimitive(QStyle::PE_PanelTipLabel, &option, &p, this);
        p.setPen(ThemeManager::tokens().color.bg);
        p.drawText(option.rect, Qt::AlignCenter, text);
    }
};

// --- Сборка страницы ---------------------------------------------------------

QWidget *card(const QString &title, QLayout *body, QWidget *parent)
{
    auto *box = new QWidget(parent);
    box->setProperty("surface", "panel");
    box->setProperty("borders", "top bottom left right");
    auto *label = new QLabel(title, box);
    label->setProperty("role", "sectionLabel");
    auto *layout = new QVBoxLayout(box);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(spacing::l);
    layout->addWidget(label);
    layout->addLayout(body);
    return box;
}

QWidget *captioned(QWidget *control, const QString &caption, QWidget *parent)
{
    auto *cell = new QWidget(parent);
    auto *label = new QLabel(caption, cell);
    label->setProperty("role", "muted");
    label->setAlignment(Qt::AlignCenter);
    auto *layout = new QVBoxLayout(cell);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(spacing::s);
    layout->addWidget(control, 0, Qt::AlignHCenter);
    layout->addWidget(label);
    return cell;
}

QHBoxLayout *row()
{
    auto *layout = new QHBoxLayout;
    layout->setSpacing(22);
    return layout;
}

QPushButton *button(const QString &icon, const QString &text, const char *variant, QWidget *parent,
                    const char *forced = nullptr)
{
    auto *b = new QPushButton(icon.isEmpty() ? QIcon() : Icons::get(icon), text, parent);
    b->setProperty("variant", variant);
    if (forced)
        b->setProperty("forceState", forced);
    return b;
}

QWidget *segment(const char *kind, const QStringList &labels, int checked, QWidget *parent, bool mono = false)
{
    auto *container = new QWidget(parent);
    container->setProperty("segmented", kind);
    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    auto *group = new QButtonGroup(container);
    for (int i = 0; i < labels.size(); ++i) {
        auto *b = new QPushButton(labels[i], container);
        b->setCheckable(true);
        b->setChecked(i == checked);
        b->setProperty("mono", mono);
        group->addButton(b, i);
        layout->addWidget(b);
    }
    return container;
}

QWidget *commandField(const char *validation, const QString &text, const QString &hint, QWidget *parent)
{
    auto *box = new QWidget(parent);
    auto *time = new QLabel(QStringLiteral("0:28:22"), box);
    time->setProperty("role", "timecode");
    auto *edit = new QLineEdit(text, box);
    edit->setPlaceholderText(QStringLiteral("13s+"));
    edit->setProperty("size", "input");
    edit->setProperty("validation", validation);
    edit->setFixedWidth(210);
    auto *hintLabel = new QLabel(hint, box);
    hintLabel->setProperty("validation", validation);
    auto *layout = new QGridLayout(box);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(time, 0, 0);
    layout->addWidget(edit, 0, 1);
    layout->addWidget(hintLabel, 1, 1);
    return box;
}

QTableView *historySample(QWidget *parent)
{
    auto *entries = new vs::EntryModel(parent);
    entries->setSourceDuration(6760'000);
    for (auto [ms, set, text] : {std::tuple{3'000, 1, u"4r!"}, std::tuple{42'000, 1, u"7s+"},
                                 std::tuple{47'000, 1, u"9a#"}, std::tuple{71'000, 2, u"13s-"}})
        entries->addEntry(ms, set, *vs::parseCommand(text).command);
    entries->setTrim(10'000, 6760'000); // первая запись — «вне видео»

    auto *proxy = new vs::EntryProxyModel(parent);
    proxy->setSourceModel(entries);

    auto *view = new QTableView(parent);
    view->setModel(proxy);
    view->setItemDelegate(new SetSeparatorDelegate(view));
    view->setItemDelegateForColumn(vs::EntryModel::SetColumn, new SetDelegate(view));
    view->setColumnHidden(vs::EntryModel::DescriptionColumn, true);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->verticalHeader()->hide();
    view->horizontalHeader()->setStretchLastSection(false);
    view->horizontalHeader()->setSectionResizeMode(vs::EntryModel::CommandColumn, QHeaderView::Stretch);
    view->setColumnWidth(vs::EntryModel::TimeColumn, 84);
    view->setColumnWidth(vs::EntryModel::SetColumn, 64);
    view->setColumnWidth(vs::EntryModel::RemoveColumn, 32);
    view->selectRow(2);
    view->setFixedSize(300, 4 * 36 + 28);
    return view;
}

QTableView *sheetSample(QWidget *parent)
{
    QList<vs::Command> commands;
    for (const char *text : {"4r#", "4r#", "4r!", "4r=", "13r#", "13r+", "13r-"})
        commands << *vs::parseCommand(QString::fromLatin1(text)).command;

    auto *model = new vs::StatsTableModel(parent);
    model->setSheet(vs::StatsSheet::build(vs::computeStats(commands)));

    auto *view = new QTableView(parent);
    view->setProperty("role", "sheet");
    view->setModel(model);
    view->setItemDelegate(new SheetDelegate(view));
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    for (const vs::SheetCell &cell : model->spans())
        view->setSpan(cell.row, cell.column, cell.rowSpan, cell.columnSpan);
    for (int column = 0; column < model->columnCount(); ++column)
        view->setColumnHidden(column, column >= 2 && (column < 7 || column > 12)); // только группа r
    view->setColumnWidth(0, 30);
    view->setColumnWidth(1, 90);
    for (int column = 7; column <= 12; ++column)
        view->setColumnWidth(column, 40);
    view->setProperty("highlightPlayer", 13);
    view->setCurrentIndex(model->index(4, 9));
    view->setFixedSize(420, 7 * 26 + 30);
    return view;
}

} // namespace

StyleGallery::StyleGallery(QWidget *parent)
    : QScrollArea(parent)
{
    setWindowTitle(QStringLiteral("VolleyScout — компоненты"));
    setWidgetResizable(true);

    auto *page = new QWidget(this);
    page->setProperty("surface", "bg");
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(32, 28, 32, 36);
    layout->setSpacing(18);

    // Заголовок + переключатель темы.
    auto *title = new QLabel(page);
    title->setFont(Fonts::ui(26, 600));
    auto updateTitle = [title] {
        title->setText(QStringLiteral("Компоненты · %1").arg(ThemeManager::tokens().name));
    };
    updateTitle();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, title, updateTitle);
    auto *toggle = button(QStringLiteral("theme-night"), QStringLiteral("Сменить тему"), "secondary", page);
    connect(toggle, &QPushButton::clicked, [] { ThemeManager::instance().toggle(); });
    auto *head = new QHBoxLayout;
    head->addWidget(title);
    head->addStretch();
    head->addWidget(toggle);
    layout->addLayout(head);

    // Цвета.
    {
        auto *body = row();
        body->addWidget(new SwatchGrid(page));
        layout->addWidget(card(QStringLiteral("Цветовые токены"), body, page));
    }
    // Кнопки.
    {
        auto *body = row();
        body->addWidget(captioned(button("table", "Сформировать таблицу", "primary", page), "primary", page));
        body->addWidget(captioned(button("table", "Hover", "primary", page, "hover"), "primary · hover", page));
        body->addWidget(captioned(button("table", "Pressed", "primary", page, "pressed"), "primary · pressed", page));
        body->addWidget(captioned(button("table", "Focus", "primary", page, "focus"), "primary · focus", page));
        body->addWidget(captioned(button("scouts-archive", "Сформировать скауты", "outline", page), "outline", page));
        body->addWidget(captioned(button("trim-start", "Начало здесь", "secondary", page), "secondary", page));
        body->addWidget(captioned(button("trim-start", "Hover", "secondary", page, "hover"), "secondary · hover", page));
        auto *disabled = button("trim-start", "Disabled", "secondary", page);
        disabled->setEnabled(false);
        body->addWidget(captioned(disabled, "disabled", page));
        body->addStretch();
        layout->addWidget(card(QStringLiteral("Кнопки (QPushButton / QToolButton, property \"variant\")"), body, page));
    }
    // Иконочные кнопки.
    {
        auto *body = row();
        body->addWidget(captioned(button("seek-back", {}, "ghost", page), "icon · ghost", page));
        auto *play = button("play", {}, "primary", page);
        play->setProperty("wide", true);
        body->addWidget(captioned(play, "icon · primary", page));
        auto *reset = button("trim-reset", {}, "ghost", page);
        reset->setProperty("size", "small");
        reset->setProperty("iconRole", "muted");
        body->addWidget(captioned(reset, "icon · muted", page));
        body->addWidget(captioned(button("close", {}, "flat", page), "icon · flat", page));
        auto *danger = button("close", {}, "flat", page, "hover");
        danger->setProperty("danger", true);
        body->addWidget(captioned(danger, "flat · hover (danger)", page));
        body->addStretch();
        layout->addWidget(card(QStringLiteral("Иконочные кнопки"), body, page));
    }
    // Сегменты и чипы.
    {
        auto *pill = new QWidget(page);
        pill->setProperty("segmented", "pill");
        auto *pillLayout = new QHBoxLayout(pill);
        for (auto [icon, on] : {std::pair{"theme-day", true}, std::pair{"theme-night", false}}) {
            auto *b = new QToolButton(pill);
            b->setIcon(Icons::get(QLatin1String(icon), Icons::Role::Muted));
            b->setIconSize(QSize(metric::iconLarge, metric::iconLarge));
            b->setCheckable(true);
            b->setChecked(on);
            pillLayout->addWidget(b);
        }
        auto *body = row();
        body->addWidget(captioned(pill, "переключатель темы", page));
        body->addWidget(captioned(segment("modeTabs", {"ФОРМИРОВАНИЕ", "ТАБЛИЦА"}, 0, page), "вкладки режима", page));
        body->addWidget(captioned(segment("sets", {"s1", "s2", "s3", "s4", "s5"}, 1, page), "текущая партия", page));
        body->addWidget(captioned(segment("chips", {"Все", "s1", "s2", "s3"}, 0, page), "фильтр (chips)", page));
        body->addWidget(captioned(segment("tabs", {"История", "Команда"}, 0, page), "вкладки панели", page));
        body->addStretch();
        layout->addWidget(card(QStringLiteral("Сегменты и чипы"), body, page));
    }
    // Поля ввода.
    {
        auto edit = [page](const QString &text, const char *forced = nullptr, bool enabled = true) {
            auto *e = new QLineEdit(text, page);
            e->setPlaceholderText(QStringLiteral("Команда 1"));
            e->setFixedWidth(200);
            e->setEnabled(enabled);
            if (forced)
                e->setProperty("forceState", forced);
            return e;
        };
        auto *date = new QDateEdit(QDate::currentDate(), page);
        date->setDisplayFormat(QStringLiteral("dd.MM.yyyy"));
        date->setCalendarPopup(true);
        date->setProperty("mono", true);
        auto *combo = new QComboBox(page);
        combo->addItems({QStringLiteral("1×"), QStringLiteral("2×")});
        auto *body = row();
        body->addWidget(captioned(edit(QStringLiteral("Команда 1")), "normal", page));
        body->addWidget(captioned(edit({}), "placeholder", page));
        body->addWidget(captioned(edit(QStringLiteral("Команда 1"), "focus"), "focus", page));
        body->addWidget(captioned(edit(QStringLiteral("Команда 1"), nullptr, false), "disabled", page));
        body->addWidget(captioned(date, "QDateEdit", page));
        body->addWidget(captioned(combo, "QComboBox", page));
        body->addStretch();
        layout->addWidget(card(QStringLiteral("Поля ввода (QLineEdit / QDateEdit / QComboBox)"), body, page));
    }
    // Поле команды.
    {
        auto *body = row();
        body->addWidget(captioned(commandField("none", {}, QStringLiteral("Enter — записать в s2"), page), "пусто", page));
        body->addWidget(captioned(commandField("ok", QStringLiteral("13s+"), QStringLiteral("✓ №13 · подача · хорошо"), page), "valid", page));
        body->addWidget(captioned(commandField("warn", QStringLiteral("13x"), QStringLiteral("Формат: номер + s r a b d + # + ! - ="), page), "incomplete", page));
        body->addWidget(captioned(commandField("error", QStringLiteral("13s!"), QStringLiteral("Оценка «!» только у r и d"), page), "error", page));
        body->addStretch();
        layout->addWidget(card(QStringLiteral("Поле команды (property \"validation\": none | ok | warn | error)"), body, page));
    }
    // История и лист.
    {
        auto *pair = new QHBoxLayout;
        pair->setSpacing(18);
        auto *history = row();
        history->addWidget(historySample(page));
        history->addStretch();
        pair->addWidget(card(QStringLiteral("Строки истории (QTableView / делегат)"), history, page), 1);
        auto *sheet = row();
        sheet->addWidget(sheetSample(page));
        sheet->addStretch();
        pair->addWidget(card(QStringLiteral("Лист статистики (QTableView + QHeaderView)"), sheet, page), 1);
        layout->addLayout(pair);
    }
    // Таймлайн и прочее.
    {
        auto *timeline = new TimelineWidget(page);
        timeline->setDuration(100'000);
        timeline->setPosition(42'000);
        QList<TimelineWidget::Marker> markers;
        const std::pair<int, vs::Grade> marks[] = {
            {4, vs::Grade::Positive}, {5, vs::Grade::Perfect}, {6, vs::Grade::Perfect}, {11, vs::Grade::Negative},
            {12, vs::Grade::Average}, {13, vs::Grade::Error}, {20, vs::Grade::Positive}, {22, vs::Grade::Positive},
            {30, vs::Grade::Perfect}, {38, vs::Grade::Negative}, {40, vs::Grade::Positive}, {55, vs::Grade::Positive},
            {57, vs::Grade::Average}, {58, vs::Grade::Negative}, {70, vs::Grade::Perfect}, {83, vs::Grade::Positive}};
        for (auto [percent, grade] : marks)
            markers.append({percent * 1000, grade});
        timeline->setMarkers(markers);
        auto *trim = new TrimBar(page);
        trim->setRange(100'000, 8'000, 88'000);
        trim->setPosition(42'000);
        auto *timelineBox = new QVBoxLayout;
        timelineBox->addWidget(timeline);
        timelineBox->addWidget(trim);
        auto *timelineWidget = new QWidget(page);
        timelineWidget->setLayout(timelineBox);
        timelineWidget->setFixedWidth(560);

        auto *timelineRow = row();
        timelineRow->addWidget(timelineWidget);
        timelineRow->addStretch();

        auto scrollbar = [page](const char *forced) {
            auto *bar = new QScrollBar(Qt::Vertical, page);
            bar->setRange(0, 100);
            bar->setPageStep(40);
            bar->setValue(20);
            bar->setFixedHeight(120);
            if (forced)
                bar->setProperty("forceState", forced);
            return bar;
        };
        auto *badge = new QToolButton(page);
        badge->setProperty("role", "badge");
        badge->setIcon(Icons::get(QStringLiteral("lock"), Icons::Role::Accent));
        badge->setIconSize(QSize(12, 12));
        badge->setText(QStringLiteral("Только чтение"));
        badge->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

        auto *other = row();
        other->addWidget(scrollbar(nullptr));
        other->addWidget(scrollbar("hover"));
        other->addWidget(new TooltipSample(page), 0, Qt::AlignTop);
        other->addWidget(badge, 0, Qt::AlignTop);
        other->addStretch();

        auto *pair = new QHBoxLayout;
        pair->setSpacing(18);
        pair->addWidget(card(QStringLiteral("Таймлайн и обрезка (кастомный виджет, рисует по токенам)"), timelineRow, page), 7);
        pair->addWidget(card(QStringLiteral("Прочее"), other, page), 5);
        layout->addLayout(pair);
    }
    // Оценки и иконки.
    {
        auto *grades = row();
        grades->addWidget(new GradeRow(page));
        grades->addStretch();
        layout->addWidget(card(QStringLiteral("Цвета оценок"), grades, page));
        auto *icons = row();
        icons->addWidget(new IconGrid(page));
        layout->addWidget(card(QStringLiteral("Иконки · resources/icons/*.svg (currentColor)"), icons, page));
    }
    layout->addStretch();

    setWidget(page);
    resize(1440, 900);
}
