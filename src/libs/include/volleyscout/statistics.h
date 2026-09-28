#pragma once

#include "volleyscout/command.h"

#include <QList>

#include <array>

namespace vs {

struct ActionStats
{
    int total = 0;
    std::array<int, kAllGrades.size()> byGrade{}; // индекс — int(Grade)

    int count(Grade grade) const { return byGrade[std::size_t(grade)]; }
    // Доля оценки от всех действий этого типа, 0..1.
    double share(Grade grade) const { return total ? double(count(grade)) / total : 0.0; }
};

struct PlayerStats
{
    int player = 0;
    std::array<ActionStats, kAllActions.size()> actions{}; // индекс — int(Action)

    const ActionStats &operator[](Action action) const { return actions[std::size_t(action)]; }
};

// Статистика по игрокам, отсортированная по номеру игрока.
QList<PlayerStats> computeStats(const QList<Command> &commands);

} // namespace vs
