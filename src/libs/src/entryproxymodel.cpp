#include "volleyscout/entryproxymodel.h"
#include "volleyscout/entrymodel.h"

namespace vs {

EntryProxyModel::EntryProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true); // пересортировка при смене партии у записи и при добавлении
}

void EntryProxyModel::setSourceModel(QAbstractItemModel *model)
{
    QSortFilterProxyModel::setSourceModel(model);
    sort(0, Qt::AscendingOrder);
}

void EntryProxyModel::setSetFilter(int set)
{
    if (m_set == set)
        return;
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    beginFilterChange();
    m_set = set;
    endFilterChange(Direction::Rows);
#else
    m_set = set;
    invalidateRowsFilter();
#endif
}

bool EntryProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (m_set == 0)
        return true;
    const QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    return index.data(EntryModel::SetRole).toInt() == m_set;
}

bool EntryProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const int leftSet = left.data(EntryModel::SetRole).toInt();
    const int rightSet = right.data(EntryModel::SetRole).toInt();
    if (leftSet != rightSet)
        return leftSet < rightSet;

    const qint64 leftTime = left.data(EntryModel::SourceTimeRole).toLongLong();
    const qint64 rightTime = right.data(EntryModel::SourceTimeRole).toLongLong();
    if (leftTime != rightTime)
        return leftTime < rightTime;

    return left.row() < right.row(); // одинаковый таймкод — в порядке ввода
}

} // namespace vs
