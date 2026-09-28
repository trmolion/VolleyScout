#include "timelinewidget.h"
#include "theme/thememanager.h"

#include <QMouseEvent>
#include <QPainter>

namespace {

constexpr int kOverhang = 4;   // плейхед выступает над полосой сверху и снизу
constexpr int kPlayheadWidth = 6;
constexpr int kMarkerWidth = 2;

// Маркер позиции плеера — красный прямоугольник (цвет danger темы).
void drawPlayhead(QPainter &p, int centerX, int height)
{
    p.fillRect(QRect(centerX - kPlayheadWidth / 2, 0, kPlayheadWidth, height), ThemeManager::tokens().color.danger);
}

} // namespace

TimelineWidget::TimelineWidget(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setCursor(Qt::PointingHandCursor);
}

void TimelineWidget::setDuration(qint64 ms)
{
    m_duration = qMax<qint64>(0, ms);
    update();
}

void TimelineWidget::setPosition(qint64 ms)
{
    const int oldX = xAt(m_position);
    m_position = ms;
    if (xAt(m_position) != oldX) // перерисовка только при сдвиге хотя бы на пиксель
        update();
}

void TimelineWidget::setMarkers(const QList<Marker> &markers)
{
    m_markers = markers;
    update();
}

QSize TimelineWidget::sizeHint() const
{
    return {400, tokens::metric::timelineBarHeight + 2 * kOverhang};
}

void TimelineWidget::paintEvent(QPaintEvent *)
{
    const ThemeTokens &t = ThemeManager::tokens();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Полоса 28px: фон inputBg, рамка border, радиус chip.
    const QRectF bar = QRectF(rect()).adjusted(0.5, kOverhang + 0.5, -0.5, -kOverhang - 0.5);
    p.setPen(QPen(t.color.border, 1));
    p.setBrush(t.color.inputBg);
    p.drawRoundedRect(bar, tokens::radius::chip, tokens::radius::chip);

    if (m_duration <= 0)
        return;

    p.setRenderHint(QPainter::Antialiasing, false);
    const int markerTop = kOverhang + 5;
    const int markerHeight = height() - 2 * kOverhang - 10;
    for (const Marker &marker : m_markers)
        p.fillRect(QRect(xAt(marker.ms), markerTop, kMarkerWidth, markerHeight), t.gradeColor(marker.grade));

    drawPlayhead(p, xAt(m_position), height());
}

void TimelineWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_duration > 0)
        emit seekRequested(msAt(event->position().toPoint().x()));
}

void TimelineWidget::mouseMoveEvent(QMouseEvent *event)
{
    if ((event->buttons() & Qt::LeftButton) && m_duration > 0)
        emit seekRequested(msAt(event->position().toPoint().x()));
}

qint64 TimelineWidget::msAt(int x) const
{
    const double ratio = qBound(0.0, double(x) / qMax(1, width() - 1), 1.0);
    return qint64(ratio * double(m_duration));
}

int TimelineWidget::xAt(qint64 ms) const
{
    if (m_duration <= 0)
        return 0;
    return int(double(qBound<qint64>(0, ms, m_duration)) / double(m_duration) * (width() - 1));
}

TrimBar::TrimBar(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void TrimBar::setRange(qint64 sourceDuration, qint64 start, qint64 end)
{
    if (m_sourceDuration == sourceDuration && m_start == start && m_end == end)
        return;
    m_sourceDuration = sourceDuration;
    m_start = start;
    m_end = end;
    update();
}

void TrimBar::setPosition(qint64 ms)
{
    if (m_sourceDuration <= 0) {
        m_position = ms;
        return;
    }
    const auto x = [this](qint64 v) { return int(double(v) * width() / double(m_sourceDuration)); };
    const int oldX = x(m_position);
    m_position = ms;
    if (x(m_position) != oldX)
        update();
}

QSize TrimBar::sizeHint() const
{
    return {200, tokens::metric::trimBarHeight + kOverhang};
}

void TrimBar::paintEvent(QPaintEvent *)
{
    const ThemeTokens &t = ThemeManager::tokens();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Полоса 10px: trimCut — отрезанное, accent — оставшееся.
    const QRectF bar(0, (height() - tokens::metric::trimBarHeight) / 2.0, width(), tokens::metric::trimBarHeight);
    p.setPen(Qt::NoPen);
    p.setBrush(t.color.trimCut);
    p.drawRoundedRect(bar, 3, 3);
    if (m_sourceDuration <= 0)
        return;

    const double scale = double(width()) / double(m_sourceDuration);
    const qreal left = double(m_start) * scale;
    const qreal right = double(qMin(m_end, m_sourceDuration)) * scale;
    p.setBrush(t.color.accent);
    p.drawRoundedRect(QRectF(left, bar.top(), qMax<qreal>(1, right - left), bar.height()), 3, 3);

    p.setRenderHint(QPainter::Antialiasing, false);
    drawPlayhead(p, int(double(m_position) * scale), height());
}
