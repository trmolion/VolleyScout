#include "screenshots.h"
#include "mainwindow.h"
#include "stylegallery.h"
#include "tablepage.h"
#include "theme/thememanager.h"

#include "volleyscout/entrymodel.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QPushButton>
#include <QTemporaryDir>

namespace {

void settle()
{
    for (int i = 0; i < 5; ++i)
        QApplication::processEvents();
}

void clickButton(QWidget *root, const QString &text)
{
    for (QPushButton *button : root->findChildren<QPushButton *>()) {
        if (button->text() == text) {
            button->click();
            return;
        }
    }
}

void fillDemo(MainWindow &window, const QString &scoutPath)
{
    auto *entries = window.findChild<vs::EntryModel *>();
    entries->setSourceDuration(6760'000);
    const std::tuple<int, int, const char *> demo[] = {
        {42, 1, "7s+"}, {44, 1, "13r#"}, {47, 1, "9a#"}, {71, 1, "13s-"}, {75, 1, "4r!"},
        {78, 1, "11a="}, {102, 1, "17b+"}, {105, 1, "9d+"}, {131, 1, "13s#"}, {160, 1, "7r-"},
        {163, 1, "11a+"}, {1685, 2, "4s+"}, {1690, 2, "13d!"}, {1694, 2, "9a-"}};
    for (auto [seconds, set, text] : demo)
        entries->addEntry(seconds * 1000LL, set, *vs::parseCommand(QString::fromLatin1(text)).command);

    window.findChild<TablePage *>()->openPath(scoutPath);
}

} // namespace

int runScreenshots(const QString &directory)
{
    QDir().mkpath(directory);
    QTemporaryDir temp;
    const QString scoutPath = temp.filePath(QStringLiteral("22.09.2026_Команда 1 vs Команда 2_s1.txt"));
    {
        QFile scout(scoutPath);
        if (!scout.open(QIODevice::WriteOnly))
            return 1;
        scout.write("7s+\n13r#\n9a#\n13s-\n4r!\n11a=\n17b+\n9d+\n13s#\n7r-\n11a+\n4d#\n17s+\n13r+\n9a+\n11b-\n"
                    "7s=\n4r#\n11a#\n13d-\n9s+\n7r!\n17a-\n9b#\n4s-\n13r-\n11a+\n7d!\n9s#\n17r+\n13a#\n11d=\n"
                    "4b+\n7s+\n9r#\n17a+\n13s+\n11r-\n9a=\n4d+\n");
    }

    const ThemeManager::Theme initial = ThemeManager::instance().current();

    MainWindow window;
    window.setAttribute(Qt::WA_DontShowOnScreen);
    window.resize(1440, 900);
    window.show();
    fillDemo(window, scoutPath);

    StyleGallery gallery;
    gallery.setWidgetResizable(false); // снимок всей страницы, а не видимой части
    gallery.setAttribute(Qt::WA_DontShowOnScreen);
    gallery.show();

    for (auto [theme, suffix] : {std::pair{ThemeManager::Theme::Day, "day"}, std::pair{ThemeManager::Theme::Night, "night"}}) {
        ThemeManager::instance().setTheme(theme);
        settle();

        clickButton(&window, QStringLiteral("ФОРМИРОВАНИЕ"));
        settle();
        window.grab().save(QDir(directory).filePath(QStringLiteral("formirovanie_%1.png").arg(QLatin1String(suffix))));

        clickButton(&window, QStringLiteral("ТАБЛИЦА"));
        settle();
        window.grab().save(QDir(directory).filePath(QStringLiteral("tablica_%1.png").arg(QLatin1String(suffix))));

        QWidget *content = gallery.content();
        content->resize(1440, content->sizeHint().height());
        settle();
        content->grab().save(QDir(directory).filePath(QStringLiteral("components_%1.png").arg(QLatin1String(suffix))));
    }
    ThemeManager::instance().setTheme(initial); // скриншоты не меняют сохранённый выбор темы
    return 0;
}
