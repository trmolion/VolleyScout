#include "volleyscout/entrymodel.h"
#include "volleyscout/textformat.h"

#include <algorithm>
#include <limits>

namespace vs {

EntryModel::EntryModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int EntryModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_entries.size());
}

int EntryModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant EntryModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid))
        return {};

    const ScoutEntry &entry = m_entries.at(index.row());
    const bool inRange = isInRange(entry.sourceMs);

    switch (role) {
    case SourceTimeRole:  return entry.sourceMs;
    case TrimmedTimeRole: return inRange ? QVariant(toTrimmed(entry.sourceMs)) : QVariant();
    case InRangeRole:     return inRange;
    case SetRole:         return entry.set;
    case PlayerRole:      return entry.command.player;
    case GradeRole:       return int(entry.command.grade);
    default:              break;
    }

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case TimeColumn:
            return inRange ? formatTime(toTrimmed(entry.sourceMs)) : QStringLiteral("вне видео");
        case SetColumn:
            return role == Qt::EditRole ? QVariant(entry.set) : QVariant(QStringLiteral("s%1").arg(entry.set));
        case CommandColumn:     return entry.command.toString();
        case DescriptionColumn: return entry.command.describe();
        case RemoveColumn:      return QStringLiteral("✕");
        }
    }

    if (role == Qt::ToolTipRole) {
        switch (index.column()) {
        case TimeColumn:
            return inRange ? QStringLiteral("Перейти к таймкоду")
                           : QStringLiteral("Таймкод вне обрезанного видео");
        case SetColumn:    return QStringLiteral("Номер партии (двойной клик — изменить)");
        case CommandColumn:
        case DescriptionColumn: return entry.command.describe();
        case RemoveColumn: return QStringLiteral("Удалить запись");
        }
    }

    return {};
}

QVariant EntryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractTableModel::headerData(section, orientation, role);

    switch (section) {
    case TimeColumn:        return QStringLiteral("Время");
    case SetColumn:         return QStringLiteral("Партия");
    case CommandColumn:     return QStringLiteral("Команда");
    case DescriptionColumn: return QStringLiteral("Расшифровка");
    case RemoveColumn:      return QString();
    }
    return {};
}

Qt::ItemFlags EntryModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f = QAbstractTableModel::flags(index);
    if (index.isValid() && index.column() == SetColumn)
        f |= Qt::ItemIsEditable;
    return f;
}

bool EntryModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole || index.column() != SetColumn
        || !checkIndex(index, CheckIndexOption::IndexIsValid))
        return false;

    bool ok = false;
    const int set = value.toInt(&ok);
    if (!ok || set < 1 || set > kMaxSets)
        return false;

    ScoutEntry &entry = m_entries[index.row()];
    if (entry.set == set)
        return true;

    entry.set = set;
    emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole, SetRole});
    return true;
}

QList<ScoutEntry> EntryModel::entriesInRange() const
{
    QList<ScoutEntry> result;
    std::copy_if(m_entries.cbegin(), m_entries.cend(), std::back_inserter(result),
                 [this](const ScoutEntry &e) { return isInRange(e.sourceMs); });
    return result;
}

int EntryModel::outOfRangeCount() const
{
    return int(std::count_if(m_entries.cbegin(), m_entries.cend(),
                             [this](const ScoutEntry &e) { return !isInRange(e.sourceMs); }));
}

int EntryModel::addEntry(qint64 sourceMs, int set, const Command &command)
{
    // upper_bound: записи с одинаковым таймкодом остаются в порядке ввода.
    const auto it = std::upper_bound(m_entries.cbegin(), m_entries.cend(), sourceMs,
                                     [](qint64 ms, const ScoutEntry &e) { return ms < e.sourceMs; });
    const int row = int(it - m_entries.cbegin());

    beginInsertRows({}, row, row);
    m_entries.insert(row, ScoutEntry{m_nextId++, sourceMs, qBound(1, set, kMaxSets), command});
    endInsertRows();
    return row;
}

void EntryModel::removeEntry(int row)
{
    if (row < 0 || row >= m_entries.size())
        return;
    beginRemoveRows({}, row, row);
    m_entries.removeAt(row);
    endRemoveRows();
}

void EntryModel::clear()
{
    beginResetModel();
    m_entries.clear();
    endResetModel();
}

void EntryModel::setSourceDuration(qint64 ms)
{
    m_sourceDuration = qMax<qint64>(0, ms);
    // Новое видео — старая обрезка к нему не относится.
    m_trimStart = 0;
    m_trimEnd = -1;
    emitTimesChanged();
}

qint64 EntryModel::trimEnd() const
{
    if (m_trimEnd >= 0)
        return m_trimEnd;
    return m_sourceDuration > 0 ? m_sourceDuration : std::numeric_limits<qint64>::max() / 2;
}

bool EntryModel::isTrimmed() const
{
    return m_trimStart > 0 || m_trimEnd >= 0;
}

void EntryModel::setTrim(qint64 startMs, qint64 endMs)
{
    const qint64 limit = m_sourceDuration > 0 ? m_sourceDuration : trimEnd();
    startMs = qBound<qint64>(0, startMs, limit);
    endMs = qBound<qint64>(startMs, endMs, limit);

    m_trimStart = startMs;
    m_trimEnd = (m_sourceDuration > 0 && endMs >= m_sourceDuration) ? -1 : endMs;
    emitTimesChanged();
}

void EntryModel::resetTrim()
{
    m_trimStart = 0;
    m_trimEnd = -1;
    emitTimesChanged();
}

bool EntryModel::isInRange(qint64 sourceMs) const
{
    return sourceMs >= m_trimStart && sourceMs <= trimEnd();
}

void EntryModel::emitTimesChanged()
{
    if (!m_entries.isEmpty()) {
        emit dataChanged(index(0, 0), index(rowCount() - 1, ColumnCount - 1),
                         {Qt::DisplayRole, Qt::ToolTipRole, TrimmedTimeRole, InRangeRole});
    }
    emit trimChanged();
}

} // namespace vs
