#pragma once

#include <QStyledItemDelegate>

// Ячейки листа статистики по токенам sheet.*: шапка, группы s r a b d с линией groupLine
// слева, оценки цветами оценок, проценты во второй строке игрока, подсветка игрока,
// выделенная ячейка — рамка 2px sheet.selection.
//
// Подсвеченный игрок берётся из свойства вида «highlightPlayer» (номер игрока или −1).
class SheetDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};
