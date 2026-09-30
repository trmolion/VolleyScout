#pragma once

#include "theme/icons.h"

#include <QIconEngine>

// Движок QIcon: SVG из qrc, currentColor → цвет текущей темы для (Mode, State).
// Рендер через QSvgRenderer с учётом devicePixelRatio; кэш — в Icons::pixmap().
class SvgIconEngine : public QIconEngine
{
public:
    SvgIconEngine(const QString &name, Icons::Role role);

    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override;
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
    QPixmap scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale) override;
    QSize actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
    QIconEngine *clone() const override;
    QString key() const override;
    QString iconName() override;

    QColor colorFor(QIcon::Mode mode, QIcon::State state) const;

private:
    QString m_name;
    Icons::Role m_role;
};
