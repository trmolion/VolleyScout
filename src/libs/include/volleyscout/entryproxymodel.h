#pragma once

#include <QSortFilterProxyModel>

namespace vs {

// Представление истории ввода поверх EntryModel: записи упорядочены по партии,
// внутри партии — по таймкоду; при необходимости показывается только одна партия.
class EntryProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit EntryProxyModel(QObject *parent = nullptr);

    void setSourceModel(QAbstractItemModel *model) override;

    int setFilter() const { return m_set; }
    // 0 — все партии.
    void setSetFilter(int set);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    int m_set = 0;
};

} // namespace vs
