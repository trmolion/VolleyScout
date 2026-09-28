#pragma once

#include "volleyscout/command.h"

#include <QAbstractTableModel>
#include <QList>

namespace vs {

// Список действий из загруженного скаута (режим «Таблица»), только чтение.
class CommandListModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column { IndexColumn, CommandColumn, DescriptionColumn, ColumnCount };
    enum Role { PlayerRole = Qt::UserRole + 1 };

    explicit CommandListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    const QList<Command> &commands() const { return m_commands; }
    void setCommands(const QList<Command> &commands);

private:
    QList<Command> m_commands;
};

} // namespace vs
