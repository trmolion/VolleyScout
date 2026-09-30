#include "theme/icons.h"
#include "theme/svgiconengine.h"
#include "theme/thememanager.h"

#include <QFile>
#include <QHash>
#include <QPainter>
#include <QSvgRenderer>

namespace Icons {

namespace {

QHash<QString, QByteArray> &sources()
{
    static QHash<QString, QByteArray> svg;
    return svg;
}

QHash<QString, QPixmap> &cache()
{
    static QHash<QString, QPixmap> pixmaps;
    return pixmaps;
}

const QByteArray &source(const QString &name)
{
    QHash<QString, QByteArray> &all = sources();
    auto it = all.find(name);
    if (it == all.end()) {
        QFile file(QStringLiteral(":/icons/%1.svg").arg(name));
        it = all.insert(name, file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray());
        Q_ASSERT_X(!it->isEmpty(), "Icons", qPrintable(name));
    }
    return *it;
}

} // namespace

QColor roleColor(Role role)
{
    const ThemeTokens::Colors &c = ThemeManager::tokens().color;
    switch (role) {
    case Role::Text:     return c.text;
    case Role::Muted:    return c.textMuted;
    case Role::Accent:   return c.accent;
    case Role::OnAccent: return c.onAccent;
    case Role::Danger:   return c.danger;
    }
    return c.text;
}

QIcon get(const QString &name, Role role)
{
    return QIcon(new SvgIconEngine(name, role));
}

QPixmap pixmap(const QString &name, int size, const QColor &color, qreal devicePixelRatio)
{
    if (size <= 0)
        return {};

    // Тема входит в ключ через цвет; кэш дополнительно чистится при смене темы.
    const QString key = QStringLiteral("%1|%2|%3|%4").arg(name).arg(size).arg(color.name(QColor::HexArgb)).arg(devicePixelRatio);
    if (const auto it = cache().constFind(key); it != cache().constEnd())
        return *it;

    QByteArray svg = source(name);
    svg.replace("currentColor", color.name(QColor::HexRgb).toLatin1());

    const int device = qRound(size * devicePixelRatio);
    QPixmap pm(device, device);
    pm.fill(Qt::transparent);
    {
        QPainter painter(&pm);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setOpacity(color.alphaF()); // альфа отдельно: SVG 1.1 не понимает #AARRGGBB
        QSvgRenderer(svg).render(&painter, QRectF(0, 0, device, device));
    }
    pm.setDevicePixelRatio(devicePixelRatio);
    cache().insert(key, pm);
    return pm;
}

QString nameOf(const QIcon &icon)
{
    return icon.isNull() ? QString() : icon.name();
}

void clearCache()
{
    cache().clear();
}

} // namespace Icons
