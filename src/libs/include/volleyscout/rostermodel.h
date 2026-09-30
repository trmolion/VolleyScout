#pragma once

#include <QAbstractTableModel>
#include <QList>
#include <QMap>

namespace vs {

// Состав команды: номер игрока → ФИО.
//
// Строки — игроки, встречающиеся в загруженных скаутах; ФИО редактируется.
// Имена хранятся по номеру и переживают смену списка игроков (переключение партий).
class RosterModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column { NumberColumn, NameColumn, ColumnCount };
    enum Role { PlayerRole = Qt::UserRole + 1 };

    explicit RosterModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    // Номера сортируются, повторы отбрасываются.
    void setPlayers(QList<int> players);

    // Введённое ФИО или пустая строка.
    QString name(int player) const { return m_names.value(player); }
    // Для таблицы статистики: ФИО, а пока его нет — «Игрок N».
    QString displayName(int player) const;

signals:
    void nameChanged(int player);

private:
    QList<int> m_players;
    QMap<int, QString> m_names;
};

} // namespace vs
