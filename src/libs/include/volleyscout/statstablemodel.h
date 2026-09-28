#pragma once

#include "volleyscout/statssheet.h"

#include <QAbstractTableModel>
#include <QList>

namespace vs {

// Таблица статистики в виде листа «как в Excel»: заголовки A, B, C… и 1, 2, 3…
// Только чтение; объединённые ячейки отдаются через spans() для QTableView::setSpan.
class StatsTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Role {
        CellKindRole = Qt::UserRole + 1, // int(vs::CellKind)
        GradeRole,                       // int(vs::Grade), если есть
        PlayerRole,                      // номер игрока для строк игроков
    };

    explicit StatsTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    const StatsSheet &sheet() const { return m_sheet; }
    void setSheet(const StatsSheet &sheet);

    QList<SheetCell> spans() const;
    // Строки таблицы, относящиеся к игроку.
    QList<int> playerRows(int player) const;

    // «A1», «AB12»…
    static QString columnName(int column);
    static QString cellReference(int row, int column);

private:
    const SheetCell *cellAt(int row, int column) const;

    StatsSheet m_sheet;
    QList<int> m_grid; // row * columnCount + column → индекс в m_sheet.cells или −1
};

} // namespace vs
