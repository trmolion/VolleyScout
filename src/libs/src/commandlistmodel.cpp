#include "volleyscout/commandlistmodel.h"

namespace vs {

CommandListModel::CommandListModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int CommandListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_commands.size());
}

int CommandListModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant CommandListModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid))
        return {};

    const Command &command = m_commands.at(index.row());
    if (role == PlayerRole)
        return command.player;
    if (role != Qt::DisplayRole)
        return {};

    switch (index.column()) {
    case IndexColumn:       return index.row() + 1;
    case CommandColumn:     return command.toString();
    case DescriptionColumn: return command.describeAction();
    }
    return {};
}

QVariant CommandListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractTableModel::headerData(section, orientation, role);

    switch (section) {
    case IndexColumn:       return QStringLiteral("№");
    case CommandColumn:     return QStringLiteral("Команда");
    case DescriptionColumn: return QStringLiteral("Расшифровка");
    }
    return {};
}

void CommandListModel::setCommands(const QList<Command> &commands)
{
    beginResetModel();
    m_commands = commands;
    endResetModel();
}

} // namespace vs
