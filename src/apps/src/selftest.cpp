#include "selftest.h"
#include "screenshots.h"
#include "theme/icons.h"
#include "theme/thememanager.h"

#include "volleyscout/entrymodel.h"
#include "volleyscout/scoutfiles.h"
#include "volleyscout/statssheet.h"
#include "volleyscout/xlsxexport.h"

#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFontDatabase>
#include <QIcon>
#include <QImageReader>
#include <QMediaPlayer>
#include <QTextStream>
#include <QVideoFrame>
#include <QVideoSink>

#include <private/qzipreader_p.h>

#include <cstdio>

namespace {

class Report
{
public:
    explicit Report(const QString &path)
        : m_file(path)
    {
        if (!m_file.open(QIODevice::WriteOnly | QIODevice::Text))
            std::fprintf(stderr, "selftest: не удалось открыть %s\n", qPrintable(path));
    }

    void check(const char *name, bool ok, const QString &details = {}, bool counts = true)
    {
        const QString line = QStringLiteral("SELFTEST %1: %2%3")
                                 .arg(QLatin1String(name), ok ? QStringLiteral("OK") : QStringLiteral("FAIL"),
                                      details.isEmpty() ? QString() : QStringLiteral(" — ") + details);
        std::fprintf(stdout, "%s\n", line.toUtf8().constData());
        std::fflush(stdout);
        QTextStream(&m_file) << line << '\n';
        m_file.flush();
        if (!ok && counts)
            ++m_failures;
    }

    int failures() const { return m_failures; }

private:
    QFile m_file;
    int m_failures = 0;
};

vs::Command cmd(const char *text)
{
    return *vs::parseCommand(QString::fromLatin1(text)).command;
}

void checkPlugins(Report &report)
{
    report.check("platform", !QGuiApplication::platformName().isEmpty(), QGuiApplication::platformName());

    const QList<QByteArray> formats = QImageReader::supportedImageFormats();
    report.check("imageformat-svg", formats.contains("svg"));

    const QIcon icon(QStringLiteral(":/icons/play.svg")); // через iconengines/qsvgicon
    report.check("iconengine-svg", !icon.isNull() && !icon.pixmap(16, 16).isNull());

    const QStringList families = QFontDatabase::families();
    report.check("font-ibm-plex-sans", families.contains(QStringLiteral("IBM Plex Sans")));
    report.check("font-jetbrains-mono", families.contains(QStringLiteral("JetBrains Mono")));
}

void checkTheme(Report &report)
{
    ThemeManager &manager = ThemeManager::instance();
    const ThemeManager::Theme initial = manager.current();
    bool ok = true;
    QStringList details;
    for (ThemeManager::Theme theme : {ThemeManager::Theme::Night, ThemeManager::Theme::Day}) {
        manager.setTheme(theme);
        const QColor window = QApplication::palette().color(QPalette::Window);
        const bool match = window == ThemeManager::tokens().color.bg;
        const QImage icon = Icons::pixmap(QStringLiteral("play"), 16, ThemeManager::tokens().color.text, 1.0).toImage();
        bool iconOk = false;
        for (int y = 0; y < icon.height() && !iconOk; ++y)
            for (int x = 0; x < icon.width() && !iconOk; ++x)
                iconOk = icon.pixelColor(x, y).alpha() > 0;
        ok = ok && match && iconOk;
        details << QStringLiteral("%1: окно %2, иконка %3")
                       .arg(ThemeManager::tokens().name, window.name(), iconOk ? QStringLiteral("есть") : QStringLiteral("нет"));
    }
    manager.setTheme(initial);
    report.check("theme-switch", ok, details.join(QStringLiteral("; ")));
}

void checkExports(Report &report, const QString &directory)
{
    // Те же функции, что вызывают кнопки «Сформировать таблицу» и «Сформировать скауты».
    vs::MatchInfo match;
    match.date = QDate(2026, 9, 22);
    vs::EntryModel entries;
    entries.addEntry(1000, 1, cmd("13s+"));
    entries.addEntry(2000, 1, cmd("7r#"));
    entries.addEntry(3000, 2, cmd("9a="));
    const vs::CommandsBySet sets = vs::commandsBySet(entries.entriesInRange());

    const QString xlsxPath = QDir(directory).filePath(match.baseName() + QStringLiteral(".xlsx"));
    QString error;
    QList<vs::XlsxSheet> sheets{{QStringLiteral("Матч"), vs::StatsSheet::build(vs::computeStats(vs::allCommands(sets)))}};
    for (auto it = sets.cbegin(); it != sets.cend(); ++it)
        sheets.append({QStringLiteral("s%1").arg(it.key()), vs::StatsSheet::build(vs::computeStats(it.value()))});
    const bool written = vs::exportStatsXlsx(xlsxPath, sheets, &error);
    QZipReader xlsx(xlsxPath);
    QStringList parts;
    for (const QZipReader::FileInfo &info : xlsx.fileInfoList())
        parts << info.filePath;
    report.check("export-xlsx", written && parts.contains(QStringLiteral("xl/workbook.xml"))
                                    && parts.contains(QStringLiteral("xl/worksheets/sheet3.xml")),
                 written ? QStringLiteral("%1, %2 частей").arg(QFileInfo(xlsxPath).fileName()).arg(parts.size()) : error);

    const QString zipPath = QDir(directory).filePath(match.baseName() + QStringLiteral(".zip"));
    const bool archived = vs::writeScoutArchive(zipPath, match, sets, &error);
    const auto scouts = vs::readScoutArchive(zipPath, &error);
    const bool roundTrip = scouts && scouts->size() == 2 && scouts->at(0).commands.size() == 2
                           && scouts->at(1).commands == QList<vs::Command>{cmd("9a=")};
    report.check("export-scouts-zip", archived && roundTrip,
                 archived ? QStringLiteral("%1, партий: %2").arg(QFileInfo(zipPath).fileName()).arg(scouts ? scouts->size() : 0)
                          : error);
}

void checkVideo(Report &report, const QString &videoPath)
{
    if (videoPath.isEmpty() || !QFileInfo::exists(videoPath)) {
        report.check("video-playback", false, QStringLiteral("не передан файл видео"));
        return;
    }

    QMediaPlayer player;
    QVideoSink sink;
    player.setVideoSink(&sink);
    int frames = 0;
    QString error;
    QObject::connect(&sink, &QVideoSink::videoFrameChanged, [&](const QVideoFrame &frame) {
        if (frame.isValid())
            ++frames;
    });
    QObject::connect(&player, &QMediaPlayer::errorOccurred,
                     [&](QMediaPlayer::Error, const QString &text) { error = text; });

    player.setSource(QUrl::fromLocalFile(videoPath));
    player.play();
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 20'000 && frames < 25 && error.isEmpty())
        QApplication::processEvents(QEventLoop::AllEvents, 50);

    report.check("video-playback", frames >= 25 && error.isEmpty(),
                 QStringLiteral("кадров %1 за %2 мс, длительность %3 мс, позиция %4 мс%5")
                     .arg(frames).arg(timer.elapsed()).arg(player.duration()).arg(player.position())
                     .arg(error.isEmpty() ? QString() : QStringLiteral(", ошибка: ") + error));
    player.stop();
}

} // namespace

int runSelfTest(const QString &directory, const QString &videoPath)
{
    QDir().mkpath(directory);
    Report report(QDir(directory).filePath(QStringLiteral("selftest.txt")));

    checkPlugins(report);
    checkTheme(report);
    checkExports(report, directory);
    checkVideo(report, videoPath);

    const QString screens = QDir(directory).filePath(QStringLiteral("screens"));
    const int screenshots = runScreenshots(screens);
    const QStringList pngs = QDir(screens).entryList({QStringLiteral("*.png")});
    report.check("ui-screenshots", screenshots == 0 && pngs.size() == 6, pngs.join(QStringLiteral(", ")));

    report.check("summary", report.failures() == 0, QStringLiteral("провалов: %1").arg(report.failures()), false);
    return report.failures();
}
