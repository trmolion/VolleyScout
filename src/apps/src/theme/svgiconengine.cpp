#include "theme/svgiconengine.h"

#include <QPainter>

SvgIconEngine::SvgIconEngine(const QString &name, Icons::Role role)
    : m_name(name)
    , m_role(role)
{
}

QColor SvgIconEngine::colorFor(QIcon::Mode mode, QIcon::State state) const
{
    if (mode == QIcon::Disabled) {
        QColor color = Icons::roleColor(Icons::Role::Muted);
        color.setAlphaF(0.5);
        return color;
    }
    if (mode == QIcon::Selected || state == QIcon::On)
        return Icons::roleColor(Icons::Role::OnAccent);
    return Icons::roleColor(m_role); // Normal и Active (hover) — цвет роли
}

void SvgIconEngine::paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state)
{
    const qreal dpr = painter->device() ? painter->device()->devicePixelRatioF() : 1.0;
    const int side = qMin(rect.width(), rect.height());
    const QPixmap pm = Icons::pixmap(m_name, side, colorFor(mode, state), dpr);
    const QRect target(rect.x() + (rect.width() - side) / 2, rect.y() + (rect.height() - side) / 2, side, side);
    painter->drawPixmap(target, pm);
}

QPixmap SvgIconEngine::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    return scaledPixmap(size, mode, state, 1.0);
}

QPixmap SvgIconEngine::scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale)
{
    return Icons::pixmap(m_name, qMin(size.width(), size.height()), colorFor(mode, state), scale);
}

QSize SvgIconEngine::actualSize(const QSize &size, QIcon::Mode, QIcon::State)
{
    const int side = qMin(size.width(), size.height());
    return {side, side};
}

QIconEngine *SvgIconEngine::clone() const
{
    return new SvgIconEngine(m_name, m_role);
}

QString SvgIconEngine::key() const
{
    return QStringLiteral("BeachSvgIconEngine");
}

QString SvgIconEngine::iconName()
{
    return m_name;
}
