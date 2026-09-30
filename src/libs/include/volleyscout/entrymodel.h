#pragma once

#include "volleyscout/scoutentry.h"

#include <QAbstractTableModel>
#include <QList>

namespace vs {

// История ввода режима «Формирование».
//
// Записи хранят таймкод исходного видео и отсортированы по нему. Обрезка видео
// задаётся диапазоном [trimStart, trimEnd] исходного видео: отображаемый таймкод
// записи = sourceMs − trimStart. Записи вне диапазона остаются в модели
// (обрезку можно сбросить), но помечаются как «вне видео» и не попадают в выгрузку.
class EntryModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column { TimeColumn, SetColumn, CommandColumn, DescriptionColumn, RemoveColumn, ColumnCount };

    enum Role {
        SourceTimeRole = Qt::UserRole + 1, // qint64
        TrimmedTimeRole,                   // qint64; невалидно, если запись вне видео
        InRangeRole,                       // bool
        SetRole,                           // int
        PlayerRole,                        // int
        GradeRole,                         // int(vs::Grade)
    };

    explicit EntryModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    const QList<ScoutEntry> &entries() const { return m_entries; }
    QList<ScoutEntry> entriesInRange() const;
    int outOfRangeCount() const;

    // Возвращает строку, в которую вставлена запись.
    int addEntry(qint64 sourceMs, int set, const Command &command);
    void removeEntry(int row);
    void clear();

    // --- Обрезка -------------------------------------------------------------
    // Длительность исходного видео; 0 — неизвестна (видео не загружено).
    qint64 sourceDuration() const { return m_sourceDuration; }
    void setSourceDuration(qint64 ms);

    qint64 trimStart() const { return m_trimStart; }
    qint64 trimEnd() const; // эффективный конец в координатах исходного видео
    qint64 trimmedDuration() const { return trimEnd() - m_trimStart; }
    bool isTrimmed() const;

    // Новые границы задаются в координатах исходного видео.
    void setTrim(qint64 startMs, qint64 endMs);
    void resetTrim();

    bool isInRange(qint64 sourceMs) const;
    qint64 toTrimmed(qint64 sourceMs) const { return sourceMs - m_trimStart; }
    qint64 toSource(qint64 trimmedMs) const { return trimmedMs + m_trimStart; }

signals:
    void trimChanged();

private:
    void emitTimesChanged();

    QList<ScoutEntry> m_entries;
    quint64 m_nextId = 1;
    qint64 m_sourceDuration = 0;
    qint64 m_trimStart = 0;
    qint64 m_trimEnd = -1; // -1 — до конца исходного видео
};

} // namespace vs
