#include "setdelegate.h"
#include "theme/fonts.h"
#include "theme/icons.h"
#include "theme/thememanager.h"

#include "volleyscout/command.h"
#include "volleyscout/entrymodel.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QPainter>

namespace {

constexpr int kSeparatorWidth = 3;

// Команда «13s+»: номер — text, действие — accent, оценка — цвет оценки.
void paintCommand(QPainter *p, const QRect &rect, const vs::Command &command, int alignment)
{
    const ThemeTokens &t = ThemeManager::tokens();
    const QFont font = Fonts::mono(tokens::font::input, 600);
    const QFontMetrics fm(font);
    const QString number = QString::number(command.player);
    const QString action(vs::actionCode(command.action));
    const QString grade(vs::gradeCode(command.grade));

    const int width = fm.horizontalAdvance(number + action + grade);
    int x = (alignment & Qt::AlignHCenter) ? rect.x() + (rect.width() - width) / 2 : rect.x() + tokens::spacing::xs;
    p->setFont(font);
    for (const auto &[text, color] : {std::pair{number, t.color.text}, std::pair{action, t.color.accent},
                                      std::pair{grade, t.gradeColor(command.grade)}}) {
        p->setPen(color);
        const int w = fm.horizontalAdvance(text);
        p->drawText(QRect(x, rect.y(), w + 1, rect.height()), Qt::AlignLeft | Qt::AlignVCenter, text);
        x += w;
    }
}

// Строка под курсором мыши — подсвечиваем всю строку, а не одну ячейку.
bool rowHovered(const QStyleOptionViewItem &option, const QModelIndex &index)
{
    const auto *view = qobject_cast<const QAbstractItemView *>(option.widget);
    if (!view || !view->viewport()->underMouse())
        return false;
    const QPoint pos = view->viewport()->mapFromGlobal(QCursor::pos());
    return view->indexAt(pos).row() == index.row();
}

} // namespace

void SetSeparatorDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);
    const bool cellHovered = opt.state & QStyle::State_MouseOver;
    paintBackground(painter, opt, index);

    const ThemeTokens &t = ThemeManager::tokens();
    const QRect content = opt.rect.adjusted(tokens::spacing::s, 0, -tokens::spacing::s, 0);
    const QVariant inRange = index.data(vs::EntryModel::InRangeRole);
    const bool outside = inRange.isValid() && !inRange.toBool();
    const QString text = index.data().toString();

    painter->save();
    if (outside)
        painter->setOpacity(0.6);

    const bool timeCell = inRange.isValid() && index.column() == vs::EntryModel::TimeColumn;
    const vs::ParseResult command = vs::parseCommand(text);

    if (timeCell) {
        // Таймкод — ссылка (клик перематывает), «вне видео» — danger без подчёркивания.
        QFont font = Fonts::mono(tokens::font::small);
        font.setUnderline(!outside);
        painter->setFont(font);
        painter->setPen(outside ? t.color.danger : t.color.link);
        painter->drawText(content, Qt::AlignLeft | Qt::AlignVCenter, text);
    } else if (text == QStringLiteral("✕")) {
        const int side = 13;
        const QColor color = (cellHovered ? t.color.danger : t.color.textMuted);
        const QRect icon(opt.rect.center().x() - side / 2, opt.rect.center().y() - side / 2, side, side);
        painter->drawPixmap(icon, Icons::pixmap(QStringLiteral("close"), side, color, painter->device()->devicePixelRatioF()));
    } else if (command.ok()) {
        paintCommand(painter, content, *command.command, Qt::AlignLeft);
    } else {
        // № строки и расшифровка в «Таблице».
        const bool number = index.column() == 0 && !inRange.isValid();
        painter->setFont(number ? Fonts::mono(tokens::font::small) : Fonts::ui(tokens::font::small));
        painter->setPen(number ? t.color.textMuted : t.color.text);
        painter->drawText(content, Qt::AlignLeft | Qt::AlignVCenter,
                          painter->fontMetrics().elidedText(text, Qt::ElideRight, content.width()));
    }
    painter->restore();

    paintSeparator(painter, opt, index);
}

QSize SetSeparatorDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const QSize base = QStyledItemDelegate::sizeHint(option, index);
    const QString text = index.data().toString();
    const QVariant inRange = index.data(vs::EntryModel::InRangeRole);

    QFont font = Fonts::ui(tokens::font::small);
    if (inRange.isValid() && index.column() == vs::EntryModel::TimeColumn)
        font = Fonts::mono(tokens::font::small);
    else if (vs::parseCommand(text).ok())
        font = Fonts::mono(tokens::font::input, 600);
    else if (index.column() == 0 && !inRange.isValid())
        font = Fonts::mono(tokens::font::small);
    if (text == QStringLiteral("✕"))
        return {32, base.height()};

    const int width = QFontMetrics(font).horizontalAdvance(text) + 2 * tokens::spacing::s + 2;
    return {width, base.height()};
}

void SetSeparatorDelegate::paintBackground(QPainter *painter, QStyleOptionViewItem &option,
                                           const QModelIndex &index) const
{
    if (rowHovered(option, index))
        option.state |= QStyle::State_MouseOver;
    else
        option.state &= ~QStyle::State_MouseOver;
    option.text.clear();
    option.icon = QIcon();
    const QWidget *widget = option.widget;
    QStyle *style = widget ? widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &option, painter, widget);
}

void SetSeparatorDelegate::paintSeparator(QPainter *painter, const QStyleOptionViewItem &option,
                                          const QModelIndex &index) const
{
    if (!startsNewSet(index))
        return;
    painter->fillRect(QRect(option.rect.left(), option.rect.top(), option.rect.width(), kSeparatorWidth),
                      ThemeManager::tokens().color.accent);
}

bool SetSeparatorDelegate::startsNewSet(const QModelIndex &index)
{
    if (!index.isValid() || index.row() == 0)
        return false;
    const QModelIndex previous = index.sibling(index.row() - 1, index.column());
    return previous.data(vs::EntryModel::SetRole) != index.data(vs::EntryModel::SetRole);
}

QWidget *SetDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const
{
    auto *combo = new QComboBox(parent);
    for (int set = 1; set <= vs::kMaxSets; ++set)
        combo->addItem(QStringLiteral("s%1").arg(set), set);

    // Выбор сразу применяется, без лишнего клика мимо редактора.
    connect(combo, &QComboBox::activated, this, [this, combo] {
        emit const_cast<SetDelegate *>(this)->commitData(combo);
        emit const_cast<SetDelegate *>(this)->closeEditor(combo);
    });
    return combo;
}

void SetDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    auto *combo = static_cast<QComboBox *>(editor);
    combo->setCurrentIndex(combo->findData(index.data(Qt::EditRole)));
}

void SetDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    auto *combo = static_cast<QComboBox *>(editor);
    model->setData(index, combo->currentData(), Qt::EditRole);
}

void SetDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);
    paintBackground(painter, opt, index);

    // Мини-комбобокс «s1 ⌄» — подсказка, что партию можно сменить.
    const ThemeTokens &t = ThemeManager::tokens();
    const int height = 24;
    const QRectF box(opt.rect.x() + tokens::spacing::xs, opt.rect.center().y() - height / 2.0 + 0.5,
                     qMin(52, opt.rect.width() - 2 * tokens::spacing::xs), height);

    painter->save();
    if (!index.data(vs::EntryModel::InRangeRole).toBool())
        painter->setOpacity(0.6);
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(t.color.border, 1));
    painter->setBrush(t.color.inputBg);
    painter->drawRoundedRect(box.adjusted(0.5, 0.5, -0.5, -0.5), tokens::radius::chip, tokens::radius::chip);

    painter->setFont(Fonts::mono(tokens::font::small));
    painter->setPen(t.color.text);
    painter->drawText(box.adjusted(tokens::spacing::xs + 2, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter,
                      index.data().toString());
    const int side = 10;
    const QRect chevron(int(box.right()) - side - tokens::spacing::xs, int(box.center().y()) - side / 2, side, side);
    painter->drawPixmap(chevron, Icons::pixmap(QStringLiteral("chevron-down"), side, t.color.textMuted,
                                               painter->device()->devicePixelRatioF()));
    painter->restore();

    paintSeparator(painter, opt, index);
}

QSize SetDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return {52 + 2 * tokens::spacing::xs, QStyledItemDelegate::sizeHint(option, index).height()};
}
