#pragma once

#include "volleyscout/statistics.h"

#include <QList>
#include <QString>
#include <QVariant>

#include <functional>
#include <optional>

namespace vs {

enum class CellKind {
    ColumnHeader, // «№», «ФИО»
    GroupHeader,  // s r a b d
    TotalHeader,  // «всего»
    GradeHeader,  // # + ! - =
    RowNumber,
    PlayerName,
    Total,
    Count,
    Percent,
};

struct SheetCell
{
    int row = 0;
    int column = 0;
    int rowSpan = 1;
    int columnSpan = 1;
    CellKind kind = CellKind::ColumnHeader;
    QVariant value; // QString, int или double (доля 0..1 для Percent); пусто — нет данных
    std::optional<Grade> grade;
    int player = -1; // номер игрока для строк игроков

    bool isHeader() const;
    QString text() const;
};

// Грейды, которые выводятся в группе действия, — в порядке убывания оценки.
QList<Grade> sheetGrades(Action action);

using PlayerNameResolver = std::function<QString(int player)>;
QString defaultPlayerName(int player);

// Таблица статистики строго по образцу:
//   строки 0–1 — шапка; далее по две строки на игрока (количество, затем проценты);
//   колонки: № | ФИО | s(всего # + - =) | r(всего # + ! - =) | a(…) | b(…) | d(всего # + ! - =).
struct StatsSheet
{
    static constexpr int kHeaderRows = 2;
    static constexpr int kRowsPerPlayer = 2;

    int rowCount = 0;
    int columnCount = 0;
    QList<SheetCell> cells;

    static StatsSheet build(const QList<PlayerStats> &stats,
                            const PlayerNameResolver &playerName = defaultPlayerName);
};

} // namespace vs
