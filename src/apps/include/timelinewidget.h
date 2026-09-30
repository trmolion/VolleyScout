#pragma once

#include "volleyscout/command.h"

#include <QList>
#include <QWidget>

// Таймлайн обрезанного видео: метки записей и позиция воспроизведения (красный маркер).
// Клик или перетаскивание — перемотка.
class TimelineWidget : public QWidget
{
    Q_OBJECT

public:
    struct Marker
    {
        qint64 ms = 0;
        vs::Grade grade = vs::Grade::Perfect;
    };

    explicit TimelineWidget(QWidget *parent = nullptr);

    void setDuration(qint64 ms);
    void setPosition(qint64 ms);
    void setMarkers(const QList<Marker> &markers);

    QSize sizeHint() const override;

signals:
    void seekRequested(qint64 ms);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    qint64 msAt(int x) const;
    int xAt(qint64 ms) const;

    qint64 m_duration = 0;
    qint64 m_position = 0;
    QList<Marker> m_markers;
};

// Полоса исходного видео с оставленным после обрезки диапазоном.
class TrimBar : public QWidget
{
    Q_OBJECT

public:
    explicit TrimBar(QWidget *parent = nullptr);

    void setRange(qint64 sourceDuration, qint64 start, qint64 end);
    // Текущая позиция плеера в исходном видео.
    void setPosition(qint64 ms);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    qint64 m_sourceDuration = 0;
    qint64 m_start = 0;
    qint64 m_end = 0;
    qint64 m_position = 0;
};
