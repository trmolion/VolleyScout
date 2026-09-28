#include "mainwindow.h"
#include "screenshots.h"
#include "stylegallery.h"
#include "theme/thememanager.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    // Оформление целиком своё: палитру и шрифты рабочего стола (Breeze, GNOME, Windows) не берём.
    QApplication::setDesktopSettingsAware(false);

    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("VolleyScout"));
    QApplication::setApplicationName(QStringLiteral("VolleyScout"));
    QApplication::setApplicationVersion(QStringLiteral(PROJECT_VERSION));

    ThemeManager::instance().install(app);

    const QStringList args = QApplication::arguments();
    if (const qsizetype i = args.indexOf(QStringLiteral("--screenshots")); i >= 0 && i + 1 < args.size())
        return runScreenshots(args.at(i + 1));

    if (args.contains(QStringLiteral("--gallery"))) {
        StyleGallery gallery;
        gallery.show();
        return app.exec();
    }

    MainWindow window;
    window.resize(1440, 900);
    window.show();
    return app.exec();
}
