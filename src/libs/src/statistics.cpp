#include "volleyscout/statistics.h"

#include <map>

namespace vs {

QList<PlayerStats> computeStats(const QList<Command> &commands)
{
    std::map<int, PlayerStats> byPlayer;
    for (const Command &command : commands) {
        PlayerStats &player = byPlayer[command.player];
        player.player = command.player;
        ActionStats &action = player.actions[std::size_t(command.action)];
        ++action.total;
        ++action.byGrade[std::size_t(command.grade)];
    }

    QList<PlayerStats> result;
    result.reserve(qsizetype(byPlayer.size()));
    for (const auto &[number, stats] : byPlayer)
        result.append(stats);
    return result;
}

} // namespace vs
