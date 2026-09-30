#include "sheetdelegate.h"
#include "theme/fonts.h"
#include "theme/thememanager.h"

#include "volleyscout/statssheet.h"
#include "volleyscout/statstablemodel.h"

#include <QAbstractItemView>
#include <QPainter>

void SheetDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    using vs::CellKind;
    const ThemeTokens &t = ThemeManager::tokens();
    const QRect r = option.rect;

    const QVariant kindValue = index.data(vs::StatsTableModel::CellKindRole);
    const auto kind = kindValue.isValid() ? CellKind(kindValue.toInt()) : CellKind::Count;
    const QVariant grade = index.data(vs::StatsTableModel::GradeRole);
    const QVariant player = index.data(vs::StatsTableModel::PlayerRole);
    const bool header = kind == CellKind::ColumnHeader || kind == CellKind::GroupHeader
                        || kind == CellKind::TotalHeader || kind == CellKind::GradeHeader;

    const QVariant highlight = option.widget ? option.widget->property("highlightPlayer") : QVariant();
    const bool highlighted = player.isValid() && highlight.isValid() && highlight.toInt() == player.toInt();

    // Фон.
    QColor background = header ? t.sheet.header : kind == CellKind::Percent ? t.sheet.cellAlt : t.sheet.cell;
    if (highlighted)
        background = t.sheet.rowHighlight;
    painter->fillRect(r, background);

    // Граница группы действия — линия groupLine слева.
    if (kind == CellKind::GroupHeader || kind == CellKind::TotalHeader || kind == CellKind::Total)
        painter->fillRect(QRect(r.left(), r.top(), 1, r.height()), t.sheet.groupLine);

    // Текст.
    QFont font = Fonts::mono(tokens::font::small);
    QColor color = t.color.text;
    switch (kind) {
    case CellKind::ColumnHeader: font = Fonts::ui(tokens::font::small, 600); break;
    case CellKind::GroupHeader:  font = Fonts::mono(tokens::font::body, 600); color = t.color.accent; break;
    case CellKind::TotalHeader:  font = Fonts::ui(tokens::font::sheetSmall); break;
    case CellKind::GradeHeader:
        font = Fonts::mono(tokens::font::small, 600);
        if (grade.isValid())
            color = t.gradeColor(vs::Grade(grade.toInt()));
        break;
    case CellKind::RowNumber:    font = Fonts::ui(tokens::font::small); break;
    case CellKind::PlayerName:   font = Fonts::ui(tokens::font::small, highlighted ? 600 : 400); break;
    case CellKind::Total:        font = Fonts::mono(tokens::font::small, 600); break;
    case CellKind::Count:        break;
    case CellKind::Percent:      font = Fonts::mono(tokens::font::sheetSmall); color = t.sheet.percentText; break;
    }

    const QString text = index.data().toString();
    if (!text.isEmpty()) {
        const QRect content = r.adjusted(tokens::spacing::xs, 0, -tokens::spacing::xs, 0);
        painter->save();
        painter->setFont(font);
        painter->setPen(color);
        const auto alignment = Qt::Alignment(index.data(Qt::TextAlignmentRole).toInt());
        painter->drawText(content, int(alignment ? alignment : Qt::AlignCenter),
                          painter->fontMetrics().elidedText(text, Qt::ElideRight, content.width()));
        painter->restore();
    }

    // Выделенная ячейка — рамка 2px.
    if (option.state & QStyle::State_Selected) {
        painter->save();
        painter->setPen(QPen(t.sheet.selection, 2));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(QRectF(r).adjusted(1, 1, -1, -1));
        painter->restore();
    }
}
