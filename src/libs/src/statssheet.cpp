#include "volleyscout/statssheet.h"

namespace vs {

bool SheetCell::isHeader() const
{
    switch (kind) {
    case CellKind::ColumnHeader:
    case CellKind::GroupHeader:
    case CellKind::TotalHeader:
    case CellKind::GradeHeader:
        return true;
    default:
        return false;
    }
}

QString SheetCell::text() const
{
    if (!value.isValid())
        return {};
    if (kind == CellKind::Percent)
        return QString::number(qRound(value.toDouble() * 100)) + u'%';
    return value.toString();
}

QList<Grade> sheetGrades(Action action)
{
    QList<Grade> grades;
    for (Grade grade : kAllGrades) {
        if (isGradeAllowed(action, grade))
            grades.append(grade);
    }
    return grades;
}

QString defaultPlayerName(int player)
{
    return QStringLiteral("Игрок %1").arg(player);
}

namespace {

SheetCell makeCell(int row, int column, int rowSpan, int columnSpan, CellKind kind, QVariant value,
                   std::optional<Grade> grade = std::nullopt)
{
    SheetCell cell;
    cell.row = row;
    cell.column = column;
    cell.rowSpan = rowSpan;
    cell.columnSpan = columnSpan;
    cell.kind = kind;
    cell.value = std::move(value);
    cell.grade = grade;
    return cell;
}

} // namespace

StatsSheet StatsSheet::build(const QList<PlayerStats> &stats, const PlayerNameResolver &playerName)
{
    StatsSheet sheet;
    auto add = [&sheet](SheetCell cell) { sheet.cells.append(std::move(cell)); };

    // --- Шапка -----------------------------------------------------------------
    add(makeCell(0, 0, 2, 1, CellKind::ColumnHeader, QStringLiteral("№")));
    add(makeCell(0, 1, 2, 1, CellKind::ColumnHeader, QStringLiteral("ФИО")));

    int column = 2;
    for (Action action : kAllActions) {
        const QList<Grade> grades = sheetGrades(action);
        const int width = 1 + int(grades.size());

        add(makeCell(0, column, 1, width, CellKind::GroupHeader, QString(actionCode(action))));
        // В образце у первой группы «Всего», у остальных — «всего».
        add(makeCell(1, column, 1, 1, CellKind::TotalHeader,
                     action == kAllActions.front() ? QStringLiteral("Всего") : QStringLiteral("всего")));
        for (int i = 0; i < grades.size(); ++i)
            add(makeCell(1, column + 1 + i, 1, 1, CellKind::GradeHeader, QString(gradeCode(grades[i])), grades[i]));
        column += width;
    }
    sheet.columnCount = column;

    // --- Игроки ----------------------------------------------------------------
    int row = kHeaderRows;
    for (int i = 0; i < stats.size(); ++i, row += kRowsPerPlayer) {
        const PlayerStats &player = stats[i];
        auto playerCell = [&](SheetCell cell) {
            cell.player = player.player;
            add(std::move(cell));
        };

        playerCell(makeCell(row, 0, 2, 1, CellKind::RowNumber, i + 1));
        playerCell(makeCell(row, 1, 2, 1, CellKind::PlayerName, playerName(player.player)));

        column = 2;
        for (Action action : kAllActions) {
            const ActionStats &s = player[action];
            const bool hasData = s.total > 0;
            const QList<Grade> grades = sheetGrades(action);

            playerCell(makeCell(row, column, 2, 1, CellKind::Total, hasData ? QVariant(s.total) : QVariant()));
            for (int g = 0; g < grades.size(); ++g) {
                const Grade grade = grades[g];
                playerCell(makeCell(row, column + 1 + g, 1, 1, CellKind::Count,
                                    hasData ? QVariant(s.count(grade)) : QVariant(), grade));
                playerCell(makeCell(row + 1, column + 1 + g, 1, 1, CellKind::Percent,
                                    hasData ? QVariant(s.share(grade)) : QVariant(), grade));
            }
            column += 1 + int(grades.size());
        }
    }
    sheet.rowCount = row;

    return sheet;
}

} // namespace vs
