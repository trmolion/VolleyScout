#pragma once

#include "volleyscout/scoutfiles.h"

#include <QList>
#include <QWidget>

class QButtonGroup;
class QLabel;
class QPushButton;
class QTableView;

namespace vs {
class CommandListModel;
class RosterModel;
class StatsTableModel;
}

// Левая часть режима «Таблица»: урезанный табличный редактор (только чтение),
// заполняемый из файла скаута «dd.mm.yyyy_1TEAM vs 2TEAM_sX.txt».
//
// Под таблицей — кнопки всех партий этого матча (соседние файлы в папке или
// файлы архива), как вкладки листов в Excel.
class TablePage : public QWidget
{
    Q_OBJECT

public:
    TablePage(vs::CommandListModel *commands, vs::RosterModel *roster, QWidget *parent = nullptr);

    // Имя текущего файла скаута (с расширением); пусто, если ничего не открыто.
    QString currentFileName() const;

public slots:
    void openScout();
    // .txt — файл партии (подтягиваются и соседние партии), .zip — архив скаутов.
    void openPath(const QString &path);
    void highlightPlayer(int player);

signals:
    void scoutShown(const QString &fileName);
    void message(const QString &text);

private:
    void loadScoutFile(const QString &path);
    void loadArchive(const QString &path);
    void setScouts(const QList<vs::ScoutFile> &scouts, int current);
    void showScout(int index);
    void rebuildSheet();
    void exportXlsx();
    void applySpans();
    void fitColumns();

    vs::CommandListModel *m_commands;
    vs::RosterModel *m_roster;
    vs::StatsTableModel *m_stats;

    QTableView *m_table;
    QPushButton *m_exportButton;
    QWidget *m_setBar;
    QButtonGroup *m_setGroup;
    QLabel *m_summary;

    QList<vs::ScoutFile> m_scouts;
    int m_current = -1;
    QString m_lastDir;
};
