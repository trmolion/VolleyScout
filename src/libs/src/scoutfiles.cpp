#include "volleyscout/scoutfiles.h"

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>

#include <private/qzipreader_p.h>
#include <private/qzipwriter_p.h>

namespace vs {

CommandsBySet commandsBySet(const QList<ScoutEntry> &entries)
{
    CommandsBySet sets;
    for (const ScoutEntry &entry : entries)
        sets[entry.set].append(entry.command);
    return sets;
}

QList<Command> allCommands(const CommandsBySet &sets)
{
    QList<Command> result;
    for (const QList<Command> &commands : sets)
        result += commands;
    return result;
}

QString scoutFileName(const MatchInfo &match, int set)
{
    return QStringLiteral("%1_s%2.txt").arg(match.baseName()).arg(set);
}

QByteArray scoutFileContent(const QList<Command> &commands)
{
    QByteArray content;
    for (const Command &command : commands)
        content += command.toString().toUtf8() + '\n';
    return content;
}

bool writeScoutArchive(const QString &zipPath, const MatchInfo &match, const CommandsBySet &sets,
                       QString *error)
{
    // QZipWriter::close() закрывает устройство, а QSaveFile закрывать нельзя —
    // поэтому архив собирается в памяти и записывается атомарно одним куском.
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    {
        QZipWriter zip(&buffer);
        zip.setCompressionPolicy(QZipWriter::AutoCompress);
        for (auto it = sets.cbegin(); it != sets.cend(); ++it)
            zip.addFile(scoutFileName(match, it.key()), scoutFileContent(it.value()));
        zip.close();
        if (zip.status() != QZipWriter::NoError) {
            if (error)
                *error = QStringLiteral("Ошибка упаковки архива");
            return false;
        }
    }

    QSaveFile file(zipPath);
    if (!file.open(QIODevice::WriteOnly) || file.write(buffer.data()) != buffer.size()
        || !file.commit()) {
        if (error)
            *error = file.errorString();
        return false;
    }
    return true;
}

namespace {

const QRegularExpression &setSuffix()
{
    static const QRegularExpression suffix(QStringLiteral("_s(\\d)$"));
    return suffix;
}

} // namespace

int setFromScoutName(const QString &baseName)
{
    const QRegularExpressionMatch m = setSuffix().match(baseName);
    return m.hasMatch() ? m.captured(1).toInt() : 0;
}

QString matchNameFromScout(const QString &baseName)
{
    QString name = baseName;
    name.remove(setSuffix());
    return name;
}

QStringList siblingScoutFiles(const QString &path)
{
    const QFileInfo info(path);
    const QString match = matchNameFromScout(info.completeBaseName());
    if (setFromScoutName(info.completeBaseName()) == 0)
        return {info.absoluteFilePath()};

    QMap<int, QString> bySet;
    for (int set = 1; set <= kMaxSets; ++set) {
        const QFileInfo sibling(info.dir(), QStringLiteral("%1_s%2.txt").arg(match).arg(set));
        if (sibling.isFile())
            bySet.insert(set, sibling.absoluteFilePath());
    }
    bySet.insert(setFromScoutName(info.completeBaseName()), info.absoluteFilePath());
    return bySet.values();
}

ScoutFile parseScout(const QByteArray &content, const QString &title)
{
    ScoutFile scout;
    scout.title = title;
    scout.set = setFromScoutName(title);

    const QList<QByteArray> lines = content.split('\n');
    for (int i = 0; i < lines.size(); ++i) {
        const QString line = QString::fromUtf8(lines[i]).trimmed();
        if (line.isEmpty())
            continue;

        const ParseResult parsed = parseCommand(line);
        if (parsed.ok())
            scout.commands.append(*parsed.command);
        else
            scout.invalidLines.append(QStringLiteral("строка %1: %2").arg(i + 1).arg(line));
    }
    return scout;
}

std::optional<ScoutFile> readScoutFile(const QString &path, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = file.errorString();
        return std::nullopt;
    }
    return parseScout(file.readAll(), QFileInfo(path).completeBaseName());
}

std::optional<QList<ScoutFile>> readScoutArchive(const QString &zipPath, QString *error)
{
    QZipReader zip(zipPath);
    if (!zip.isReadable() || zip.status() != QZipReader::NoError) {
        if (error)
            *error = QStringLiteral("Не удалось прочитать архив %1").arg(zipPath);
        return std::nullopt;
    }

    QList<ScoutFile> scouts;
    for (const QZipReader::FileInfo &entry : zip.fileInfoList()) {
        if (!entry.isFile || !entry.filePath.endsWith(QLatin1String(".txt"), Qt::CaseInsensitive))
            continue;
        scouts.append(parseScout(zip.fileData(entry.filePath), QFileInfo(entry.filePath).completeBaseName()));
    }
    std::stable_sort(scouts.begin(), scouts.end(),
                     [](const ScoutFile &a, const ScoutFile &b) { return a.set < b.set; });
    return scouts;
}

} // namespace vs
