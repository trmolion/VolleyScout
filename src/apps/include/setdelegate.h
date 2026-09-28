#pragma once

#include <QStyledItemDelegate>

// Строки истории ввода (оба режима): таймкод-ссылка, команда по частям цветами темы,
// иконка удаления; над первой строкой каждой новой партии — разделитель.
class SetSeparatorDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

protected:
    // Фон строки (выделение, hover всей строки, разделитель строк) — средствами стиля.
    void paintBackground(QPainter *painter, QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paintSeparator(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    // Строка начинает новую партию (предыдущая строка — из другой партии).
    static bool startsNewSet(const QModelIndex &index);
};

// Колонка «Партия»: редактор номера партии — выпадающий список s1…s5.
class SetDelegate : public SetSeparatorDelegate
{
    Q_OBJECT

public:
    using SetSeparatorDelegate::SetSeparatorDelegate;

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};
