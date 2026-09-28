#include "volleyscout/rostermodel.h"
#include "volleyscout/statssheet.h"

#include <algorithm>

namespace vs {

RosterModel::RosterModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int RosterModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_players.size());
}

int RosterModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant RosterModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid))
        return {};

    const int player = m_players.at(index.row());
    if (role == PlayerRole)
        return player;
    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return {};

    return index.column() == NumberColumn ? QVariant(player) : QVariant(name(player));
}

QVariant RosterModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractTableModel::headerData(section, orientation, role);
    return section == NumberColumn ? QStringLiteral("№") : QStringLiteral("ФИО");
}

Qt::ItemFlags RosterModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f = QAbstractTableModel::flags(index);
    if (index.isValid() && index.column() == NameColumn)
        f |= Qt::ItemIsEditable;
    return f;
}

bool RosterModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole || index.column() != NameColumn
        || !checkIndex(index, CheckIndexOption::IndexIsValid))
        return false;

    const int player = m_players.at(index.row());
    const QString text = value.toString().simplified();
    if (text == name(player))
        return true;

    if (text.isEmpty())
        m_names.remove(player);
    else
        m_names.insert(player, text);

    emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
    emit nameChanged(player);
    return true;
}

void RosterModel::setPlayers(QList<int> players)
{
    std::sort(players.begin(), players.end());
    players.erase(std::unique(players.begin(), players.end()), players.end());
    if (players == m_players)
        return;

    beginResetModel();
    m_players = players;
    endResetModel();
}

QString RosterModel::displayName(int player) const
{
    const QString fio = name(player);
    return fio.isEmpty() ? defaultPlayerName(player) : fio;
}

} // namespace vs
