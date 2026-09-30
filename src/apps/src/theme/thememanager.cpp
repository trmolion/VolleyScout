#include "theme/thememanager.h"
#include "theme/beachstyle.h"
#include "theme/fonts.h"
#include "theme/icons.h"

#include <QApplication>
#include <QSettings>
#include <QStyleFactory>
#include <QTimer>
#include <QWidget>

namespace {

const char *const kSettingsKey = "ui/theme";

QColor withAlpha(QColor color, qreal alpha)
{
    color.setAlphaF(alpha);
    return color;
}

} // namespace

ThemeManager &ThemeManager::instance()
{
    static ThemeManager manager;
    return manager;
}

void ThemeManager::install(QApplication &app)
{
    const QString saved = QSettings().value(QLatin1String(kSettingsKey), QStringLiteral("day")).toString();
    m_theme = saved == QLatin1String("night") ? Theme::Night : Theme::Day;

    Fonts::registerAll();
    QApplication::setStyle(new BeachStyle(QStyleFactory::create(QStringLiteral("Fusion"))));
    QApplication::setFont(Fonts::ui(tokens::font::body));
    applyPalette();

    app.installEventFilter(this);
}

void ThemeManager::setTheme(Theme theme)
{
    if (theme == m_theme)
        return;
    m_theme = theme;
    QSettings().setValue(QLatin1String(kSettingsKey), theme == Theme::Night ? QStringLiteral("night")
                                                                            : QStringLiteral("day"));
    Icons::clearCache();
    applyPalette();
    repolishAll();
    emit themeChanged(theme);
}

void ThemeManager::toggle()
{
    setTheme(m_theme == Theme::Day ? Theme::Night : Theme::Day);
}

bool ThemeManager::eventFilter(QObject *watched, QEvent *event)
{
    // Платформенная тема (например, plasma-integration при смене схемы) прислала свою
    // палитру — возвращаем свою. Отложенно: менять палитру внутри её же события нельзя.
    if (watched == qApp && !m_applying && !m_reapplyQueued
        && (event->type() == QEvent::ApplicationPaletteChange || event->type() == QEvent::ThemeChange)) {
        if (QApplication::palette() != buildPalette()) {
            m_reapplyQueued = true;
            QTimer::singleShot(0, this, [this] {
                m_reapplyQueued = false;
                applyPalette();
                repolishAll();
            });
        }
    }
    return QObject::eventFilter(watched, event);
}

QPalette ThemeManager::buildPalette() const
{
    const ThemeTokens::Colors &c = tokens().color;
    QPalette p;
    for (QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
        const bool disabled = group == QPalette::Disabled;
        const QColor text = disabled ? c.textMuted : c.text;

        p.setColor(group, QPalette::Window, c.bg);
        p.setColor(group, QPalette::WindowText, text);
        p.setColor(group, QPalette::Base, disabled ? c.panel2 : c.inputBg);
        p.setColor(group, QPalette::AlternateBase, c.panel2);
        p.setColor(group, QPalette::Text, text);
        p.setColor(group, QPalette::PlaceholderText, c.placeholder);
        p.setColor(group, QPalette::Button, c.panel);
        p.setColor(group, QPalette::ButtonText, text);
        p.setColor(group, QPalette::BrightText, c.onAccent);
        p.setColor(group, QPalette::Highlight, disabled ? c.border : c.accent);
        p.setColor(group, QPalette::HighlightedText, c.onAccent);
        p.setColor(group, QPalette::Link, c.link);
        p.setColor(group, QPalette::LinkVisited, c.link);
        p.setColor(group, QPalette::ToolTipBase, c.text);
        p.setColor(group, QPalette::ToolTipText, c.bg);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        p.setColor(group, QPalette::Accent, c.accent);
#endif

        // Объёмные роли — производные от border.
        p.setColor(group, QPalette::Light, c.panel);
        p.setColor(group, QPalette::Midlight, c.rowDivider);
        p.setColor(group, QPalette::Mid, c.border);
        p.setColor(group, QPalette::Dark, c.border.darker(130));
        p.setColor(group, QPalette::Shadow, withAlpha(c.text, 0.35));
    }
    return p;
}

void ThemeManager::applyPalette()
{
    m_applying = true;
    QApplication::setPalette(buildPalette());
    m_applying = false;
}

void ThemeManager::repolishAll()
{
    QStyle *style = QApplication::style();
    for (QWidget *widget : QApplication::allWidgets()) {
        style->unpolish(widget);
        style->polish(widget);
        widget->update();
    }
}
