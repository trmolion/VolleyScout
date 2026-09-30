#include "formationpage.h"
#include "videopanel.h"
#include "theme/icons.h"
#include "theme/themetokens.h"

#include "volleyscout/entrymodel.h"
#include "volleyscout/scoutfiles.h"
#include "volleyscout/statssheet.h"
#include "volleyscout/xlsxexport.h"

#include <QDateEdit>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>

namespace {

QString setsLabel(const vs::CommandsBySet &sets)
{
    QStringList names;
    for (int set : sets.keys())
        names << QStringLiteral("s%1").arg(set);
    return names.join(QStringLiteral(", "));
}

} // namespace

FormationPage::FormationPage(vs::EntryModel *entries, QWidget *parent)
    : QWidget(parent)
    , m_entries(entries)
    , m_video(new VideoPanel(entries, this))
    , m_date(new QDateEdit(QDate::currentDate(), this))
    , m_team1(new QLineEdit(this))
    , m_team2(new QLineEdit(this))
    , m_lastDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
{
    const vs::MatchInfo defaults;
    m_date->setDisplayFormat(QStringLiteral("dd.MM.yyyy"));
    m_date->setCalendarPopup(true);
    m_date->setToolTip(tr("Дата матча"));
    m_team1->setPlaceholderText(defaults.team1);
    m_team2->setPlaceholderText(defaults.team2);
    m_team1->setToolTip(tr("Команда 1"));
    m_team2->setToolTip(tr("Команда 2"));

    auto *scoutsButton = new QPushButton(Icons::get(QStringLiteral("scouts-archive")), tr("Сформировать скауты"), this);
    auto *tableButton = new QPushButton(Icons::get(QStringLiteral("table")), tr("Сформировать таблицу"), this);
    scoutsButton->setProperty("variant", "outline");
    tableButton->setProperty("variant", "primary");
    scoutsButton->setProperty("size", "large");
    tableButton->setProperty("size", "large");
    m_date->setProperty("mono", true);
    scoutsButton->setToolTip(tr("Архив .zip: по одному .txt на каждую партию"));
    tableButton->setToolTip(tr("Таблица статистики .xlsx и архив скаутов по партиям"));

    auto *vs = new QLabel(QStringLiteral("vs"), this);
    vs->setProperty("role", "muted");

    // --- Матч: одна строка ---
    auto *matchBar = new QWidget(this);
    matchBar->setProperty("surface", "panel");
    auto *matchRow = new QHBoxLayout(matchBar);
    matchRow->setContentsMargins(tokens::spacing::xl, tokens::spacing::l, tokens::spacing::xl, tokens::spacing::l);
    matchRow->setSpacing(10);
    matchRow->addWidget(m_date);
    matchRow->addWidget(m_team1, 1);
    matchRow->addWidget(vs);
    matchRow->addWidget(m_team2, 1);
    matchRow->addStretch(1);
    matchRow->addWidget(scoutsButton);
    matchRow->addWidget(tableButton);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_video, 1);
    layout->addWidget(matchBar);

    connect(tableButton, &QPushButton::clicked, this, &FormationPage::generateTable);
    connect(scoutsButton, &QPushButton::clicked, this, &FormationPage::generateScouts);

    connect(m_date, &QDateEdit::dateChanged, this, &FormationPage::matchInfoChanged);
    connect(m_team1, &QLineEdit::textChanged, this, &FormationPage::matchInfoChanged);
    connect(m_team2, &QLineEdit::textChanged, this, &FormationPage::matchInfoChanged);
    connect(m_video, &VideoPanel::message, this, &FormationPage::message);
}

vs::MatchInfo FormationPage::matchInfo() const
{
    vs::MatchInfo info;
    info.date = m_date->date();
    // Пустое поле — имя по умолчанию («Команда 1» / «Команда 2»), как в подсказке поля.
    if (!m_team1->text().trimmed().isEmpty())
        info.team1 = m_team1->text().trimmed();
    if (!m_team2->text().trimmed().isEmpty())
        info.team2 = m_team2->text().trimmed();
    return info;
}

void FormationPage::generateTable()
{
    if (!ensureEntries())
        return;

    const vs::MatchInfo match = matchInfo();
    const vs::CommandsBySet sets = vs::commandsBySet(m_entries->entriesInRange());

    const QString xlsxPath = askSavePath(tr("Сохранить таблицу"), match.baseName() + QStringLiteral(".xlsx"),
                                         tr("Книга Excel (*.xlsx)"));
    if (xlsxPath.isEmpty())
        return;

    // Лист «Матч» — все партии вместе, затем по листу на каждую партию.
    QList<vs::XlsxSheet> sheets{{tr("Матч"), vs::StatsSheet::build(vs::computeStats(vs::allCommands(sets)))}};
    for (auto it = sets.cbegin(); it != sets.cend(); ++it)
        sheets.append({QStringLiteral("s%1").arg(it.key()), vs::StatsSheet::build(vs::computeStats(it.value()))});

    QString error;
    if (!vs::exportStatsXlsx(xlsxPath, sheets, &error)) {
        QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось сохранить таблицу:\n%1").arg(error));
        return;
    }

    // Вместе с таблицей формируется и архив скаутов — рядом с ней.
    const QString zipPath = QFileInfo(xlsxPath).dir().filePath(match.baseName() + QStringLiteral(".zip"));
    if (!vs::writeScoutArchive(zipPath, match, sets, &error)) {
        QMessageBox::critical(this, tr("Ошибка"), tr("Таблица сохранена, но архив скаутов — нет:\n%1").arg(error));
        return;
    }

    emit message(tr("Сформировано: %1 и %2 (%3)")
                     .arg(QFileInfo(xlsxPath).fileName(), QFileInfo(zipPath).fileName(), setsLabel(sets)));
    emit tableGenerated(zipPath);
}

void FormationPage::generateScouts()
{
    if (!ensureEntries())
        return;

    const vs::MatchInfo match = matchInfo();
    const vs::CommandsBySet sets = vs::commandsBySet(m_entries->entriesInRange());

    const QString zipPath = askSavePath(tr("Сохранить скауты"), match.baseName() + QStringLiteral(".zip"),
                                        tr("Архив ZIP (*.zip)"));
    if (zipPath.isEmpty())
        return;

    QString error;
    if (!vs::writeScoutArchive(zipPath, match, sets, &error)) {
        QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось сохранить архив:\n%1").arg(error));
        return;
    }
    emit message(tr("Архив %1: %2 — по одному .txt на партию").arg(QFileInfo(zipPath).fileName(), setsLabel(sets)));
}

bool FormationPage::ensureEntries()
{
    if (!m_entries->entriesInRange().isEmpty())
        return true;

    QMessageBox::information(this, tr("Нет данных"),
                             m_entries->entries().isEmpty()
                                 ? tr("История ввода пуста — нечего выгружать.")
                                 : tr("Все записи находятся вне обрезанного видео — нечего выгружать."));
    return false;
}

QString FormationPage::askSavePath(const QString &title, const QString &fileName, const QString &filter)
{
    const QString path = QFileDialog::getSaveFileName(this, title, QDir(m_lastDir).filePath(fileName), filter);
    if (!path.isEmpty())
        m_lastDir = QFileInfo(path).absolutePath();
    return path;
}
