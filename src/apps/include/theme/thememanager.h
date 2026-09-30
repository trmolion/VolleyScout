#pragma once

#include "theme/themetokens.h"

#include <QObject>
#include <QPalette>

class QApplication;

// Текущая тема приложения («Полдень» / «Сумерки»).
//
// install() ставит BeachStyle, шрифты и палитру явно — системная тема (Breeze, GNOME,
// Windows) игнорируется. Если платформа всё же перетирает палитру (смена схемы KDE),
// своя палитра применяется заново.
class ThemeManager : public QObject
{
    Q_OBJECT

public:
    enum class Theme { Day, Night };

    static ThemeManager &instance();
    // Токены текущей темы — единственный источник цветов для стиля и виджетов.
    static const ThemeTokens &tokens() { return instance().current() == Theme::Day ? ThemeTokens::day() : ThemeTokens::night(); }

    void install(QApplication &app);

    Theme current() const { return m_theme; }
    void setTheme(Theme theme);
    void toggle();

signals:
    void themeChanged(ThemeManager::Theme theme);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    ThemeManager() = default;

    QPalette buildPalette() const;
    void applyPalette();
    void repolishAll();

    Theme m_theme = Theme::Day;
    bool m_applying = false;
    bool m_reapplyQueued = false;
};
