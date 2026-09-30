#pragma once

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QString>

// SVG-иконки из resources/icons (qrc :/icons). Цвет задаёт тема, а не файл.
namespace Icons {

// Базовый цвет иконки в обычном состоянии.
enum class Role { Text, Muted, Accent, OnAccent, Danger };

QColor roleColor(Role role);

// QIcon с SvgIconEngine: цвет по роли и состоянию (Disabled, On) из текущей темы.
QIcon get(const QString &name, Role role = Role::Text);

// Иконка заданного цвета — для стиля и делегатов. size — в логических пикселях.
QPixmap pixmap(const QString &name, int size, const QColor &color, qreal devicePixelRatio);

// Имя SVG, из которого сделан QIcon (пусто, если иконка не наша).
QString nameOf(const QIcon &icon);

// Сбрасывается ThemeManager при смене темы.
void clearCache();

} // namespace Icons
