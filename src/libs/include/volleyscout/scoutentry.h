#pragma once

#include "volleyscout/command.h"

#include <QDate>
#include <QString>

namespace vs {

// Запись истории ввода: команда, привязанная к таймкоду и партии.
struct ScoutEntry
{
    quint64 id = 0;
    qint64 sourceMs = 0; // таймкод в исходном (необрезанном) видео
    int set = 1;         // 1..kMaxSets
    Command command;
};

struct MatchInfo
{
    QDate date = QDate::currentDate();
    QString team1 = QStringLiteral("Команда 1");
    QString team2 = QStringLiteral("Команда 2");

    // «dd.mm.yyyy_1TEAM vs 2TEAM» — основа имён всех выгружаемых файлов.
    QString baseName() const;
};

} // namespace vs
