#include "tablepage.h"
#include "sheetdelegate.h"
#include "theme/icons.h"
#include "theme/themetokens.h"

#include "volleyscout/commandlistmodel.h"
#include "volleyscout/rostermodel.h"
#include "volleyscout/statstablemodel.h"
#include "volleyscout/textformat.h"
#include "volleyscout/xlsxexport.h"

#include <QButtonGroup>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QStyle>
#include <QTableView>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

constexpr int kMaxReportedLines = 15;

QString setLabel(const vs::ScoutFile &scout)
{
    return scout.set > 0 ? QStringLiteral("s%1").arg(scout.set) : scout.title;
}

} // namespace

TablePage::TablePage(vs::CommandListModel *commands, vs::RosterModel *roster, QWidget *parent)
    : QWidget(parent)
    , m_commands(commands)
    , m_roster(roster)
    , m_stats(new vs::StatsTableModel(this))
    , m_table(new QTableView(this))
    , m_exportButton(new QPushButton(tr("Экспорт .xlsx"), this))
    , m_setBar(new QWidget(this))
    , m_setGroup(new QButtonGroup(this))
    , m_summary(new QLabel(this))
    , m_lastDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
{
    using namespace tokens;

    // --- Панель инструментов ---
    auto *openButton = new QPushButton(Icons::get(QStringLiteral("folder-open")), tr("Открыть скаут…"), this);
    openButton->setProperty("variant", "primary");
    m_exportButton->setIcon(Icons::get(QStringLiteral("export")));
    m_exportButton->setEnabled(false);

    auto *readOnly = new QToolButton(this);
    readOnly->setProperty("role", "badge");
    readOnly->setIcon(Icons::get(QStringLiteral("lock"), Icons::Role::Accent));
    readOnly->setIconSize(QSize(12, 12));
    readOnly->setText(tr("Только чтение"));
    readOnly->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    readOnly->setFocusPolicy(Qt::NoFocus);
    readOnly->setAttribute(Qt::WA_TransparentForMouseEvents);

    auto *toolbarBar = new QWidget(this);
    toolbarBar->setProperty("surface", "panel");
    toolbarBar->setProperty("borders", "bottom");
    auto *toolbar = new QHBoxLayout(toolbarBar);
    toolbar->setContentsMargins(spacing::l, spacing::m, spacing::l, spacing::m);
    toolbar->addWidget(openButton);
    toolbar->addWidget(m_exportButton);
    toolbar->addStretch();
    toolbar->addWidget(readOnly);

    // --- Лист ---
    m_table->setModel(m_stats);
    m_table->setProperty("role", "sheet");
    m_table->setItemDelegate(new SheetDelegate(m_table));
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setWordWrap(false);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_table->horizontalHeader()->setMinimumSectionSize(24);
    m_table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    m_table->verticalHeader()->setDefaultSectionSize(24);

    // --- Партии матча ---
    m_setBar->setProperty("segmented", "sets");
    m_summary->setProperty("role", "muted");
    auto *setLayout = new QHBoxLayout(m_setBar);
    setLayout->setContentsMargins(0, 0, 0, 0);

    auto *bottomBar = new QWidget(this);
    bottomBar->setProperty("surface", "panel2");
    bottomBar->setProperty("borders", "top");
    auto *bottom = new QHBoxLayout(bottomBar);
    bottom->setContentsMargins(spacing::m, spacing::xs, spacing::l, spacing::xs);
    bottom->addWidget(m_setBar);
    bottom->addWidget(m_summary, 1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(toolbarBar);
    layout->addWidget(m_table, 1);
    layout->addWidget(bottomBar);

    connect(openButton, &QPushButton::clicked, this, &TablePage::openScout);
    connect(m_exportButton, &QPushButton::clicked, this, &TablePage::exportXlsx);
    connect(m_setGroup, &QButtonGroup::idClicked, this, &TablePage::showScout);
    connect(m_stats, &QAbstractItemModel::modelReset, this, &TablePage::applySpans);
    // Введено ФИО — сразу в колонку «ФИО» основной таблицы.
    connect(m_roster, &vs::RosterModel::nameChanged, this, &TablePage::rebuildSheet);

    m_summary->setText(tr("Откройте файл скаута, сформированный в режиме «Формирование»"));
}

QString TablePage::currentFileName() const
{
    if (m_current < 0 || m_current >= m_scouts.size())
        return {};
    return m_scouts[m_current].title + QStringLiteral(".txt");
}

void TablePage::openScout()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Открыть скаут"), m_lastDir,
                                                      tr("Скауты (*.txt *.zip);;Все файлы (*)"));
    if (!path.isEmpty())
        openPath(path);
}

void TablePage::openPath(const QString &path)
{
    m_lastDir = QFileInfo(path).absolutePath();
    if (path.endsWith(QLatin1String(".zip"), Qt::CaseInsensitive))
        loadArchive(path);
    else
        loadScoutFile(path);
}

void TablePage::loadScoutFile(const QString &path)
{
    const QString chosen = QFileInfo(path).absoluteFilePath();
    QList<vs::ScoutFile> scouts;
    int current = 0;

    for (const QString &file : vs::siblingScoutFiles(chosen)) {
        QString error;
        const auto scout = vs::readScoutFile(file, &error);
        if (!scout) {
            // Не открылся соседний файл — просто без его кнопки; не открылся выбранный — ошибка.
            if (file == chosen) {
                QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось открыть файл:\n%1").arg(error));
                return;
            }
            continue;
        }
        if (file == chosen)
            current = int(scouts.size());
        scouts.append(*scout);
    }
    setScouts(scouts, current);
}

void TablePage::loadArchive(const QString &path)
{
    QString error;
    const auto scouts = vs::readScoutArchive(path, &error);
    if (!scouts) {
        QMessageBox::critical(this, tr("Ошибка"), error);
        return;
    }
    if (scouts->isEmpty()) {
        QMessageBox::information(this, tr("Нет данных"), tr("В архиве нет файлов скаутов (.txt)."));
        return;
    }
    setScouts(*scouts, 0);
}

void TablePage::setScouts(const QList<vs::ScoutFile> &scouts, int current)
{
    m_scouts = scouts;

    for (QAbstractButton *button : m_setGroup->buttons()) {
        m_setGroup->removeButton(button);
        delete button;
    }
    for (int i = 0; i < m_scouts.size(); ++i) {
        auto *button = new QPushButton(setLabel(m_scouts[i]), m_setBar);
        button->setCheckable(true);
        button->setFocusPolicy(Qt::NoFocus);
        button->setToolTip(m_scouts[i].title + QStringLiteral(".txt"));
        m_setGroup->addButton(button, i);
        m_setBar->layout()->addWidget(button);
    }

    // Состав — все игроки матча, а не только текущей партии.
    QList<int> players;
    for (const vs::ScoutFile &scout : m_scouts) {
        for (const vs::Command &command : scout.commands)
            players.append(command.player);
    }
    m_roster->setPlayers(players);

    m_exportButton->setEnabled(!m_scouts.isEmpty());
    showScout(current);
}

void TablePage::showScout(int index)
{
    if (index < 0 || index >= m_scouts.size())
        return;

    m_current = index;
    m_setGroup->button(index)->setChecked(true);
    m_table->setProperty("highlightPlayer", -1);

    const vs::ScoutFile &scout = m_scouts[index];
    m_commands->setCommands(scout.commands);
    rebuildSheet();

    const int players = (m_stats->rowCount() - vs::StatsSheet::kHeaderRows) / vs::StatsSheet::kRowsPerPlayer;
    QStringList summary{vs::pluralRu(players, tr("игрок"), tr("игрока"), tr("игроков")),
                        vs::pluralRu(scout.commands.size(), tr("действие"), tr("действия"), tr("действий"))};

    // Нераспознанные строки не попадают в таблицу — счётчик в строке, сами строки в подсказке.
    QString invalidTip;
    if (const auto invalid = scout.invalidLines.size()) {
        summary << tr("не распознано строк: %1").arg(invalid);
        QStringList shown = scout.invalidLines.mid(0, kMaxReportedLines);
        if (invalid > kMaxReportedLines)
            shown << tr("… и ещё %1").arg(invalid - kMaxReportedLines);
        invalidTip = tr("Пропущены — не вошли в таблицу:\n%1").arg(shown.join(u'\n'));
    }
    summary << tr("только чтение");
    m_summary->setText(summary.join(QStringLiteral(" · ")));
    m_summary->setToolTip(invalidTip);

    emit scoutShown(currentFileName());
}

void TablePage::rebuildSheet()
{
    if (m_current < 0 || m_current >= m_scouts.size())
        return;
    const auto name = [this](int player) { return m_roster->displayName(player); };
    m_stats->setSheet(vs::StatsSheet::build(vs::computeStats(m_scouts[m_current].commands), name));
}

void TablePage::highlightPlayer(int player)
{
    // Подсветку строк игрока рисует SheetDelegate (sheet.rowHighlight); выделение ячейки — отдельно.
    m_table->setProperty("highlightPlayer", player);
    m_table->viewport()->update();

    const QList<int> rows = m_stats->playerRows(player);
    if (!rows.isEmpty())
        m_table->scrollTo(m_stats->index(rows.first(), 0));
}

void TablePage::exportXlsx()
{
    if (m_current < 0)
        return;
    const vs::ScoutFile &scout = m_scouts[m_current];

    const QString path = QFileDialog::getSaveFileName(this, tr("Экспорт таблицы"),
                                                      QDir(m_lastDir).filePath(scout.title + QStringLiteral(".xlsx")),
                                                      tr("Книга Excel (*.xlsx)"));
    if (path.isEmpty())
        return;
    m_lastDir = QFileInfo(path).absolutePath();

    // Имя листа Excel ограничено 31 символом — берём номер партии, а не имя файла.
    const QString sheetName = scout.set > 0 ? QStringLiteral("s%1").arg(scout.set) : tr("Статистика");

    QString error;
    if (!vs::exportStatsXlsx(path, {{sheetName, m_stats->sheet()}}, &error)) {
        QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось сохранить таблицу:\n%1").arg(error));
        return;
    }
    emit message(tr("Таблица сохранена: %1").arg(QFileInfo(path).fileName()));
}

void TablePage::applySpans()
{
    m_table->clearSpans();
    for (const vs::SheetCell &cell : m_stats->spans())
        m_table->setSpan(cell.row, cell.column, cell.rowSpan, cell.columnSpan);
    fitColumns();
}

void TablePage::fitColumns()
{
    // QTableView при подгонке ширины не учитывает объединённые ячейки, а колонки
    // «№» и «ФИО» состоят только из них — поэтому ширину добираем по тексту вручную.
    m_table->resizeColumnsToContents();

    const QFontMetrics metrics(m_table->font());
    const int padding = 2 * m_table->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, m_table) + 12;
    QList<int> widths(m_stats->columnCount(), 0);
    for (const vs::SheetCell &cell : m_stats->sheet().cells) {
        if (cell.columnSpan == 1)
            widths[cell.column] = qMax(widths[cell.column], metrics.horizontalAdvance(cell.text()) + padding);
    }
    for (int column = 0; column < widths.size(); ++column) {
        if (widths[column] > m_table->columnWidth(column))
            m_table->setColumnWidth(column, widths[column]);
    }
}
