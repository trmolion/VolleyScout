#pragma once

#include "volleyscout/scoutentry.h"

#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

#include <optional>

namespace vs {

// Команды по партиям (ключ — номер партии 1..5) в порядке таймкодов.
using CommandsBySet = QMap<int, QList<Command>>;
CommandsBySet commandsBySet(const QList<ScoutEntry> &entries);
QList<Command> allCommands(const CommandsBySet &sets);

// «dd.mm.yyyy_1TEAM vs 2TEAM_s1.txt»
QString scoutFileName(const MatchInfo &match, int set);
// Одна команда в строке, без таймкодов: «13s+».
QByteArray scoutFileContent(const QList<Command> &commands);

// Архив «dd.mm.yyyy_1TEAM vs 2TEAM.zip» с одним файлом на каждую партию.
bool writeScoutArchive(const QString &zipPath, const MatchInfo &match, const CommandsBySet &sets,
                       QString *error = nullptr);

struct ScoutFile
{
    QString title;           // имя файла без расширения
    int set = 0;             // номер партии из имени файла; 0 — не распознан
    QList<Command> commands; // в порядке следования в файле
    QStringList invalidLines; // «строка N: текст» — строки, которые не удалось разобрать
};

ScoutFile parseScout(const QByteArray &content, const QString &title);
std::optional<ScoutFile> readScoutFile(const QString &path, QString *error = nullptr);
// Все скауты из архива, сформированного writeScoutArchive, — по возрастанию номера партии.
std::optional<QList<ScoutFile>> readScoutArchive(const QString &zipPath, QString *error = nullptr);

// Номер партии из суффикса «_sN»; 0, если суффикса нет.
int setFromScoutName(const QString &baseName);
// Имя матча без суффикса партии: «dd.mm.yyyy_1TEAM vs 2TEAM».
QString matchNameFromScout(const QString &baseName);
// Файлы партий того же матча в папке файла path (включая его самого), по номеру партии.
QStringList siblingScoutFiles(const QString &path);

} // namespace vs
