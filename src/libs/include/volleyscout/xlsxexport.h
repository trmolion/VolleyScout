#pragma once

#include "volleyscout/statssheet.h"

#include <QList>
#include <QString>

namespace vs {

struct XlsxSheet
{
    QString name; // имя листа, например «Матч» или «s1»
    StatsSheet sheet;
};

bool exportStatsXlsx(const QString &path, const QList<XlsxSheet> &sheets, QString *error = nullptr);

} // namespace vs
