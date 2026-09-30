#pragma once

#include "volleyscout/scoutentry.h"

#include <QWidget>

class QDateEdit;
class QLineEdit;
class VideoPanel;

namespace vs {
class EntryModel;
}

// Левая часть режима «Формирование»: видео с обрезкой, параметры матча, выгрузка.
class FormationPage : public QWidget
{
    Q_OBJECT

public:
    explicit FormationPage(vs::EntryModel *entries, QWidget *parent = nullptr);

    VideoPanel *video() const { return m_video; }
    vs::MatchInfo matchInfo() const;

signals:
    void matchInfoChanged();
    void message(const QString &text);
    // Таблица и архив скаутов сохранены; zipPath — архив с файлами партий.
    void tableGenerated(const QString &zipPath);

private:
    void generateTable();
    void generateScouts();
    bool ensureEntries();
    QString askSavePath(const QString &title, const QString &fileName, const QString &filter);

    vs::EntryModel *m_entries;
    VideoPanel *m_video;
    QDateEdit *m_date;
    QLineEdit *m_team1;
    QLineEdit *m_team2;
    QString m_lastDir;
};
