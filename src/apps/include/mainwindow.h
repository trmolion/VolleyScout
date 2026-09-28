#pragma once

#include "volleyscout/command.h"

#include <QMainWindow>

class FormationPage;
class HistoryPanel;
class QButtonGroup;
class QKeyEvent;
class QLabel;
class QStackedWidget;
class TablePage;

namespace vs {
class CommandListModel;
class EntryModel;
class RosterModel;
}

// Окно: слева (4/5) меняется страница режима, справа (1/5) — общая панель истории и ввода.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    enum Mode { FormationMode, TableMode };

    QWidget *createThemeToggle(QWidget *parent);
    void setMode(int mode);
    void updateHeaderInfo();
    void addCommand(const vs::Command &command, int set);
    void showGeneratedTable(const QString &zipPath);
    bool handleSpace(QObject *watched, QKeyEvent *event);

    vs::EntryModel *m_entries;
    vs::CommandListModel *m_commands;
    vs::RosterModel *m_roster;

    QButtonGroup *m_modeGroup;
    QLabel *m_headerInfo;
    QStackedWidget *m_pages;
    FormationPage *m_formation;
    TablePage *m_table;
    HistoryPanel *m_history;
};
