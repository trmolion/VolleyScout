#include "volleyscout/xlsxexport.h"

#include "xlsxdocument.h"
#include "xlsxformat.h"
#include "xlsxworksheet.h"

namespace vs {

namespace {

QXlsx::Format cellFormat(const SheetCell &cell)
{
    QXlsx::Format format;
    format.setBorderStyle(QXlsx::Format::BorderThin);
    format.setVerticalAlignment(QXlsx::Format::AlignVCenter);
    format.setHorizontalAlignment(cell.kind == CellKind::PlayerName ? QXlsx::Format::AlignLeft
                                                                    : QXlsx::Format::AlignHCenter);
    if (cell.isHeader() || cell.kind == CellKind::Total)
        format.setFontBold(true);
    if (cell.kind == CellKind::Percent)
        format.setNumberFormat(QStringLiteral("0%"));
    return format;
}

void writeSheet(QXlsx::Worksheet *sheet, const StatsSheet &stats)
{
    // QXlsx адресует ячейки с 1.
    for (const SheetCell &cell : stats.cells) {
        const int row = cell.row + 1;
        const int column = cell.column + 1;
        const QXlsx::Format format = cellFormat(cell);

        if (cell.value.isValid())
            sheet->write(row, column, cell.value, format);
        else
            sheet->writeBlank(row, column, format);

        if (cell.rowSpan > 1 || cell.columnSpan > 1) {
            sheet->mergeCells(QXlsx::CellRange(row, column, row + cell.rowSpan - 1,
                                               column + cell.columnSpan - 1),
                              format);
        }
    }

    sheet->setColumnWidth(1, 1, 5);
    sheet->setColumnWidth(2, 2, 22);
    if (stats.columnCount > 2)
        sheet->setColumnWidth(3, stats.columnCount, 6);
}

} // namespace

bool exportStatsXlsx(const QString &path, const QList<XlsxSheet> &sheets, QString *error)
{
    QXlsx::Document document;
    for (const XlsxSheet &sheet : sheets) {
        if (!document.addSheet(sheet.name)) {
            if (error)
                *error = QStringLiteral("Не удалось создать лист «%1»").arg(sheet.name);
            return false;
        }
        writeSheet(document.currentWorksheet(), sheet.sheet);
    }
    if (!sheets.isEmpty())
        document.selectSheet(0);

    if (!document.saveAs(path)) {
        if (error)
            *error = QStringLiteral("Не удалось сохранить файл %1").arg(path);
        return false;
    }
    return true;
}

} // namespace vs
