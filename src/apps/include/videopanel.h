#pragma once

#include <QWidget>

class QAudioOutput;
class QComboBox;
class QLabel;
class QMediaPlayer;
class QPushButton;
class QTimer;
class QVideoWidget;
class TimelineWidget;
class TrimBar;

namespace vs {
class EntryModel;
}

// Видеопроигрыватель с таймлайном и обрезкой с концов.
//
// Обрезка не перекодирует файл: плеер ограничивается диапазоном [trimStart, trimEnd]
// исходного видео, а все таймкоды наружу отдаются относительно trimStart.
class VideoPanel : public QWidget
{
    Q_OBJECT

public:
    explicit VideoPanel(vs::EntryModel *entries, QWidget *parent = nullptr);

    // Позиция в исходном видео — к ней привязываются новые записи.
    qint64 sourcePosition() const;
    qint64 trimmedPosition() const;

public slots:
    void openVideo();
    void seekTrimmed(qint64 ms);
    void togglePlay();

signals:
    void positionChanged(qint64 trimmedMs);
    void message(const QString &text);

private:
    void onSourcePositionChanged(qint64 sourceMs);
    void onSourceDurationChanged(qint64 ms);
    void updateVideoInfo();
    void flushSeek();
    void trimStartHere();
    void trimEndHere();
    void resetTrim();
    void clampToTrim();
    void updateTimeline();
    void updateMarkers();

    vs::EntryModel *m_entries;

    QMediaPlayer *m_player;
    QAudioOutput *m_audio;
    QVideoWidget *m_videoWidget;

    QLabel *m_fileLabel;
    QLabel *m_videoInfo;
    QPushButton *m_playButton;
    QLabel *m_timeLabel;
    QComboBox *m_speed;
    TimelineWidget *m_timeline;
    TrimBar *m_trimBar;
    QPushButton *m_trimStartButton;
    QPushButton *m_trimEndButton;
    QPushButton *m_trimResetButton;

    bool m_awaitingDuration = false;

    // Перемотка при перетаскивании по таймлайну: запросы объединяются, чтобы не
    // заваливать декодер десятками seek в секунду — отсюда и «лаги» при скрабинге.
    QTimer *m_seekThrottle;
    qint64 m_pendingSeek = -1; // позиция в исходном видео; −1 — нет отложенной перемотки
};
