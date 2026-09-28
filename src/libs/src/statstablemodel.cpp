#include "volleyscout/statstablemodel.h"

namespace vs {

StatsTableModel::StatsTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int StatsTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_sheet.rowCount;
}

int StatsTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_sheet.columnCount;
}

QVariant StatsTableModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid))
        return {};

    const SheetCell *cell = cellAt(index.row(), index.column());
    if (!cell)
        return {};

    switch (role) {
    case Qt::DisplayRole:
        return cell->text();
    case Qt::TextAlignmentRole:
        return cell->kind == CellKind::PlayerName ? QVariant(Qt::AlignLeft | Qt::AlignVCenter)
                                                  : QVariant(Qt::AlignCenter);
    case CellKindRole:
        return int(cell->kind);
    case GradeRole:
        return cell->grade ? QVariant(int(*cell->grade)) : QVariant();
    case PlayerRole:
        return cell->player >= 0 ? QVariant(cell->player) : QVariant();
    }
    return {};
}

QVariant StatsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QAbstractTableModel::headerData(section, orientation, role);
    return orientation == Qt::Horizontal ? QVariant(columnName(section)) : QVariant(section + 1);
}

Qt::ItemFlags StatsTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable; // без Qt::ItemIsEditable — лист закрыт
}

void StatsTableModel::setSheet(const StatsSheet &sheet)
{
    beginResetModel();
    m_sheet = sheet;
    m_grid.fill(-1, qsizetype(m_sheet.rowCount) * m_sheet.columnCount);
    for (int i = 0; i < m_sheet.cells.size(); ++i) {
        const SheetCell &cell = m_sheet.cells[i];
        m_grid[cell.row * m_sheet.columnCount + cell.column] = i;
    }
    endResetModel();
}

QList<SheetCell> StatsTableModel::spans() const
{
    QList<SheetCell> result;
    for (const SheetCell &cell : m_sheet.cells) {
        if (cell.rowSpan > 1 || cell.columnSpan > 1)
            result.append(cell);
    }
    return result;
}

QList<int> StatsTableModel::playerRows(int player) const
{
    QList<int> rows;
    for (const SheetCell &cell : m_sheet.cells) {
        if (cell.player == player && cell.column == 0) {
            for (int r = 0; r < cell.rowSpan; ++r)
                rows.append(cell.row + r);
        }
    }
    return rows;
}

QString StatsTableModel::columnName(int column)
{
    QString name;
    for (int n = column + 1; n > 0; n = (n - 1) / 26)
        name.prepend(QChar(u'A' + (n - 1) % 26));
    return name;
}

QString StatsTableModel::cellReference(int row, int column)
{
    return columnName(column) + QString::number(row + 1);
}

const SheetCell *StatsTableModel::cellAt(int row, int column) const
{
    const qsizetype pos = qsizetype(row) * m_sheet.columnCount + column;
    if (pos < 0 || pos >= m_grid.size() || m_grid[pos] < 0)
        return nullptr;
    return &m_sheet.cells[m_grid[pos]];
}

} // namespace vs
