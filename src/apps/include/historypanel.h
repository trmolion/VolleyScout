#pragma once

#include "volleyscout/command.h"

#include <QWidget>

class QButtonGroup;
class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QTableView;
class SetDelegate;
class SetSeparatorDelegate;

namespace vs {
class CommandListModel;
class EntryModel;
class EntryProxyModel;
class RosterModel;
}

// Правая часть окна — общая для обоих режимов: история ввода + поле ввода команд.
//   «Формирование» — история записей с таймкодами, фильтр по партиям, ввод команд.
//   «Таблица»      — две вкладки: «История» (действия загруженного скаута в порядке записи)
//                    и «Команда» (состав: номер → ФИО); блок ввода команд скрыт.
class HistoryPanel : public QWidget
{
    Q_OBJECT

public:
    enum class Mode { Formation, Table };

    HistoryPanel(vs::EntryModel *entries, vs::CommandListModel *commands, vs::RosterModel *roster,
                 QWidget *parent = nullptr);

    void setMode(Mode mode);
    int currentSet() const;
    QWidget *commandInput() const;

public slots:
    void setCurrentTime(qint64 trimmedMs);
    void focusInput();

signals:
    void commandSubmitted(const vs::Command &command, int set);
    void seekRequested(qint64 trimmedMs);
    // Режим «Таблица»: выбран игрок для подсветки; −1 — снять подсветку.
    void playerPicked(int player);

private:
    void submit();
    void updateHint();
    void updateCounters();
    void onViewClicked(const QModelIndex &index);

    Mode m_mode = Mode::Formation;
    vs::EntryModel *m_entries;
    vs::CommandListModel *m_commands;
    vs::RosterModel *m_roster;
    vs::EntryProxyModel *m_entriesProxy;
    SetSeparatorDelegate *m_rowDelegate;
    SetDelegate *m_setDelegate;
    int m_pickedPlayer = -1;

    QWidget *m_title;          // «ИСТОРИЯ» + счётчик — режим «Формирование»
    QWidget *m_tabBar;         // «История» | «Команда» — режим «Таблица»
    QButtonGroup *m_tabGroup;
    QStackedWidget *m_pages;
    QTableView *m_rosterView;
    QLabel *m_countLabel;
    QWidget *m_filterBar;
    QButtonGroup *m_filterGroup;
    QTableView *m_view;

    QWidget *m_inputArea; // партия + поле ввода + подсказка; в «Таблице» скрыт
    QButtonGroup *m_setGroup;
    QWidget *m_setButtons;
    QLabel *m_timeLabel;
    QLineEdit *m_input;
    QPushButton *m_submitButton;
    QLabel *m_hintLabel;
};
