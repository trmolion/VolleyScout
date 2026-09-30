#include "mainwindow.h"
#include "formationpage.h"
#include "historypanel.h"
#include "tablepage.h"
#include "videopanel.h"
#include "theme/icons.h"
#include "theme/thememanager.h"

#include "volleyscout/commandlistmodel.h"
#include "volleyscout/entrymodel.h"
#include "volleyscout/rostermodel.h"

#include <QAbstractSpinBox>
#include <QApplication>
#include <QButtonGroup>
#include <QKeyEvent>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QStatusBar>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
constexpr int kMessageTimeoutMs = 6000;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_entries(new vs::EntryModel(this))
    , m_commands(new vs::CommandListModel(this))
    , m_roster(new vs::RosterModel(this))
    , m_modeGroup(new QButtonGroup(this))
    , m_headerInfo(new QLabel(this))
    , m_pages(new QStackedWidget(this))
    , m_formation(new FormationPage(m_entries, this))
    , m_table(new TablePage(m_commands, m_roster, this))
    , m_history(new HistoryPanel(m_entries, m_commands, m_roster, this))
{
    setWindowTitle(QStringLiteral("VolleyScout"));

    // --- Верхняя панель: переключатель режимов ---
    auto *headerBar = new QWidget(this);
    headerBar->setProperty("surface", "panel");
    headerBar->setProperty("borders", "bottom");
    headerBar->setFixedHeight(tokens::metric::headerHeight);

    auto *logo = new QLabel(headerBar);
    auto *brand = new QLabel(QStringLiteral("VolleyScout"), headerBar);
    brand->setProperty("role", "brand");
    auto refreshLogo = [logo] {
        logo->setPixmap(Icons::pixmap(QStringLiteral("app-logo"), 22, ThemeManager::tokens().color.accent,
                                      logo->devicePixelRatioF()));
    };
    refreshLogo();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, logo, refreshLogo);

    auto *modeTabs = new QWidget(headerBar);
    modeTabs->setProperty("segmented", "modeTabs");
    auto *modeLayout = new QHBoxLayout(modeTabs);
    for (auto [mode, title] : {std::pair{FormationMode, tr("ФОРМИРОВАНИЕ")}, std::pair{TableMode, tr("ТАБЛИЦА")}}) {
        auto *button = new QPushButton(title, modeTabs);
        button->setCheckable(true);
        button->setFocusPolicy(Qt::NoFocus);
        m_modeGroup->addButton(button, mode);
        modeLayout->addWidget(button);
    }

    m_headerInfo->setProperty("role", "muted");

    auto *header = new QHBoxLayout(headerBar);
    header->setContentsMargins(tokens::spacing::xxl, 0, tokens::spacing::xxl, 0);
    header->setSpacing(tokens::spacing::section);
    auto *brandLayout = new QHBoxLayout;
    brandLayout->setSpacing(10);
    brandLayout->addWidget(logo);
    brandLayout->addWidget(brand);
    header->addLayout(brandLayout);
    header->addWidget(modeTabs);
    header->addStretch();
    header->addWidget(m_headerInfo);
    header->addWidget(createThemeToggle(headerBar));

    // --- Основная область: 4/5 страница режима, 1/5 история и ввод ---
    m_pages->insertWidget(FormationMode, m_formation);
    m_pages->insertWidget(TableMode, m_table);

    m_history->setMinimumWidth(tokens::metric::rightPaneMinWidth);

    auto *body = new QHBoxLayout;
    body->setSpacing(0);
    body->addWidget(m_pages, tokens::metric::leftPaneStretch);
    body->addWidget(m_history, tokens::metric::rightPaneStretch);

    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(headerBar);
    layout->addLayout(body, 1);
    setCentralWidget(central);

    connect(m_modeGroup, &QButtonGroup::idClicked, this, &MainWindow::setMode);

    connect(m_history, &HistoryPanel::commandSubmitted, this, &MainWindow::addCommand);
    connect(m_history, &HistoryPanel::seekRequested, m_formation->video(), &VideoPanel::seekTrimmed);
    connect(m_history, &HistoryPanel::playerPicked, m_table, &TablePage::highlightPlayer);
    connect(m_formation->video(), &VideoPanel::positionChanged, m_history, &HistoryPanel::setCurrentTime);

    connect(m_formation, &FormationPage::matchInfoChanged, this, &MainWindow::updateHeaderInfo);
    connect(m_table, &TablePage::scoutShown, this, &MainWindow::updateHeaderInfo);
    connect(m_formation, &FormationPage::tableGenerated, this, &MainWindow::showGeneratedTable);

    auto showMessage = [this](const QString &text) { statusBar()->showMessage(text, kMessageTimeoutMs); };
    connect(m_formation, &FormationPage::message, this, showMessage);
    connect(m_table, &TablePage::message, this, showMessage);

    m_modeGroup->button(FormationMode)->setChecked(true);
    setMode(FormationMode);

    // Пробел — только «старт/пауза» плеера, где бы ни был фокус (кроме полей с текстом).
    qApp->installEventFilter(this);
}

QWidget *MainWindow::createThemeToggle(QWidget *parent)
{
    // Переключатель темы: «Полдень» | «Сумерки».
    auto *toggle = new QWidget(parent);
    toggle->setProperty("segmented", "pill");
    auto *layout = new QHBoxLayout(toggle);
    auto *group = new QButtonGroup(toggle);

    const auto current = ThemeManager::instance().current();
    for (auto [theme, icon, name] : {std::tuple{ThemeManager::Theme::Day, "theme-day", tr("Дневная тема")},
                                     std::tuple{ThemeManager::Theme::Night, "theme-night", tr("Ночная тема")}}) {
        auto *button = new QToolButton(toggle);
        button->setIcon(Icons::get(QLatin1String(icon), Icons::Role::Muted));
        button->setIconSize(QSize(tokens::metric::iconLarge, tokens::metric::iconLarge));
        button->setCheckable(true);
        button->setChecked(theme == current);
        button->setFocusPolicy(Qt::NoFocus);
        button->setToolTip(name);
        button->setAccessibleName(name);
        group->addButton(button, int(theme));
        layout->addWidget(button);
    }
    connect(group, &QButtonGroup::idClicked, this,
            [](int id) { ThemeManager::instance().setTheme(ThemeManager::Theme(id)); });
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, group,
            [group](ThemeManager::Theme theme) { group->button(int(theme))->setChecked(true); });
    return toggle;
}

void MainWindow::setMode(int mode)
{
    m_pages->setCurrentIndex(mode);
    m_history->setMode(mode == FormationMode ? HistoryPanel::Mode::Formation : HistoryPanel::Mode::Table);
    updateHeaderInfo();
    if (mode == FormationMode)
        m_history->focusInput();
}

void MainWindow::updateHeaderInfo()
{
    if (m_modeGroup->checkedId() == TableMode) {
        const QString file = m_table->currentFileName();
        m_headerInfo->setText(file.isEmpty() ? tr("Файл не загружен") : file);
        return;
    }
    const vs::MatchInfo match = m_formation->matchInfo();
    m_headerInfo->setText(QStringLiteral("%1 · %2 vs %3")
                              .arg(match.date.toString(QStringLiteral("dd.MM.yyyy")), match.team1, match.team2));
}

void MainWindow::addCommand(const vs::Command &command, int set)
{
    m_entries->addEntry(m_formation->video()->sourcePosition(), set, command);
}

void MainWindow::showGeneratedTable(const QString &zipPath)
{
    m_modeGroup->button(TableMode)->setChecked(true);
    setMode(TableMode);
    m_table->openPath(zipPath);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease
        || event->type() == QEvent::ShortcutOverride) {
        if (handleSpace(watched, static_cast<QKeyEvent *>(event)))
            return true;
    }
    return QMainWindow::eventFilter(watched, event);
}

bool MainWindow::handleSpace(QObject *watched, QKeyEvent *event)
{
    if (event->key() != Qt::Key_Space || event->modifiers() != Qt::NoModifier)
        return false;

    // Только события виджетов этого окна; диалоги (сохранение файла и т. п.) не трогаем.
    auto *widget = qobject_cast<QWidget *>(watched);
    if (!widget || widget->window() != this)
        return false;

    // В названиях команд и в дате пробел — обычный символ. Поле ввода команд — исключение:
    // в командах пробелов нет, и оттуда пробел тоже управляет плеером.
    if (widget != m_history->commandInput()
        && (qobject_cast<QLineEdit *>(widget) || qobject_cast<QAbstractSpinBox *>(widget)))
        return false;

    // ShortcutOverride принимаем, чтобы пробел не ушёл кнопкам; переключаем по первому нажатию.
    if (event->type() == QEvent::ShortcutOverride) {
        event->accept();
        return true;
    }
    if (event->type() == QEvent::KeyPress && !event->isAutoRepeat() && m_modeGroup->checkedId() == FormationMode)
        m_formation->video()->togglePlay();
    return true;
}
