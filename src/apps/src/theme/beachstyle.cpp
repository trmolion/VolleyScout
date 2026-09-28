#include "theme/beachstyle.h"
#include "theme/fonts.h"
#include "theme/icons.h"
#include "theme/thememanager.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QCalendarWidget>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QScrollBar>
#include <QStyleOption>
#include <QTableView>
#include <QToolButton>

namespace {

using namespace tokens;

const ThemeTokens &T()
{
    return ThemeManager::tokens();
}

// Смешение: доля t цвета b в цвете a.
QColor mix(const QColor &a, const QColor &b, qreal t)
{
    return QColor::fromRgbF(float(a.redF() * (1 - t) + b.redF() * t), float(a.greenF() * (1 - t) + b.greenF() * t),
                            float(a.blueF() * (1 - t) + b.blueF() * t), float(a.alphaF() * (1 - t) + b.alphaF() * t));
}

QByteArray prop(const QWidget *widget, const char *name)
{
    return widget ? widget->property(name).toByteArray() : QByteArray();
}

bool flag(const QWidget *widget, const char *name)
{
    return widget && widget->property(name).toBool();
}

QByteArray segmentOf(const QWidget *widget)
{
    return widget ? prop(widget->parentWidget(), "segmented") : QByteArray();
}

QStyle::State effectiveState(QStyle::State state, const QWidget *widget)
{
    const QByteArray forced = prop(widget, "forceState");
    if (forced == "hover")
        state |= QStyle::State_MouseOver;
    else if (forced == "pressed")
        state |= QStyle::State_Sunken | QStyle::State_MouseOver;
    else if (forced == "focus")
        state |= QStyle::State_HasFocus | QStyle::State_KeyboardFocusChange;
    return state;
}

qreal dprOf(const QPainter *painter)
{
    return painter->device() ? painter->device()->devicePixelRatioF() : qApp->devicePixelRatio();
}

// --- Кнопки -------------------------------------------------------------------

enum class Variant { Primary, Outline, Secondary, Ghost, Flat, Overlay };

Variant variantOf(const QWidget *widget, bool isDefault)
{
    const QByteArray v = prop(widget, "variant");
    if (v == "primary")  return Variant::Primary;
    if (v == "outline")  return Variant::Outline;
    if (v == "ghost")    return Variant::Ghost;
    if (v == "flat")     return Variant::Flat;
    if (v == "overlay")  return Variant::Overlay;
    if (v == "secondary") return Variant::Secondary;
    if (isDefault)
        return Variant::Primary; // кнопка по умолчанию в диалогах
    return qobject_cast<const QToolButton *>(widget) ? Variant::Ghost : Variant::Secondary;
}

struct ButtonLook
{
    QColor bg = Qt::transparent;
    QColor border = Qt::transparent;
    QColor fg;
    qreal radius = radius::control;
    qreal opacity = 1.0;
    bool focusRing = false;
};

ButtonLook buttonLook(const QWidget *widget, QStyle::State state, const QRect &rect, bool isDefault = false)
{
    const ThemeTokens::Colors &c = T().color;
    const bool enabled = state & QStyle::State_Enabled;
    const bool hover = enabled && (state & QStyle::State_MouseOver);
    const bool pressed = enabled && (state & QStyle::State_Sunken);
    const bool checked = state & QStyle::State_On;
    const qreal round = rect.height() / 2.0;

    ButtonLook look;
    look.fg = c.text;

    if (prop(widget, "role") == "badge") {
        look.bg = c.accentSoft;
        look.fg = c.accent;
        look.radius = round;
        return look;
    }

    const QByteArray segment = segmentOf(widget);
    if (!segment.isEmpty()) {
        const bool filled = segment != "chips";
        look.radius = (segment == "pill" || segment == "chips") ? round : qreal(radius::segment);
        if (checked) {
            look.bg = filled ? c.accent : c.accentSoft;
            look.border = filled ? c.accent : c.accent;
            look.fg = filled ? c.onAccent : c.accent;
            if (segment != "sets" && segment != "chips")
                look.border = Qt::transparent;
        } else {
            look.fg = hover ? c.text : c.textMuted;
            look.bg = hover ? c.accentSoft : QColor(Qt::transparent);
            if (segment == "sets" || segment == "chips")
                look.border = c.border;
        }
        if (!enabled)
            look.opacity = 0.5;
        return look;
    }

    const Variant variant = variantOf(widget, isDefault);
    switch (variant) {
    case Variant::Primary:
        look.bg = pressed ? mix(c.accent, c.text, 0.25) : hover ? mix(c.accent, c.text, 0.12) : c.accent;
        look.fg = c.onAccent;
        break;
    case Variant::Outline:
        look.border = c.accent;
        look.fg = c.accent;
        look.bg = pressed ? mix(c.accentSoft, c.accent, 0.2) : hover ? c.accentSoft : QColor(Qt::transparent);
        break;
    case Variant::Secondary:
        look.border = c.border;
        look.bg = pressed ? mix(c.accentSoft, c.border, 0.5) : hover ? c.accentSoft : c.inputBg;
        break;
    case Variant::Ghost:
        look.border = c.border;
        look.bg = pressed ? mix(c.accentSoft, c.border, 0.5) : hover ? c.accentSoft : QColor(Qt::transparent);
        break;
    case Variant::Flat:
        look.radius = radius::chip;
        look.fg = (hover && flag(widget, "danger")) ? c.danger : c.textMuted;
        look.bg = (hover || pressed) ? c.accentSoft : QColor(Qt::transparent);
        break;
    case Variant::Overlay: {
        const ThemeTokens::Overlay &o = T().overlay;
        look.bg = o.chipBg;
        if (hover)
            look.bg.setAlphaF(0.8);
        look.border = o.chipBorder;
        look.fg = o.chipText;
        break;
    }
    }

    const QByteArray iconRole = prop(widget, "iconRole");
    if (iconRole == "muted")
        look.fg = c.textMuted;
    else if (iconRole == "accent")
        look.fg = c.accent;
    else if (iconRole == "danger")
        look.fg = c.danger;

    if (!enabled) {
        look.bg = variant == Variant::Flat ? QColor(Qt::transparent) : c.panel2;
        look.border = variant == Variant::Flat ? QColor(Qt::transparent) : c.border;
        look.fg = c.textMuted;
        look.opacity = 0.5;
    }

    look.focusRing = (state & QStyle::State_HasFocus) && (state & QStyle::State_KeyboardFocusChange) && enabled;
    return look;
}

void paintPanel(QPainter *p, QRectF rect, const ButtonLook &look)
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    p->setOpacity(look.opacity);

    if (look.focusRing) {
        // Кольцо 2px accent с зазором 2px цвета фона — внутри границ виджета.
        const qreal w = metric::focusRingWidth;
        const QRectF ring = rect.adjusted(w / 2, w / 2, -w / 2, -w / 2);
        p->setPen(QPen(T().color.accent, w));
        p->setBrush(Qt::NoBrush);
        const qreal r = qMin(look.radius + 2 * w, ring.height() / 2);
        p->drawRoundedRect(ring, r, r);
        rect = rect.adjusted(2 * w, 2 * w, -2 * w, -2 * w);
    }

    const qreal radius = qMin(look.radius, rect.height() / 2);
    if (look.bg.alpha() > 0) {
        p->setPen(Qt::NoPen);
        p->setBrush(look.bg);
        p->drawRoundedRect(rect, radius, radius);
    }
    if (look.border.alpha() > 0) {
        p->setPen(QPen(look.border, 1));
        p->setBrush(Qt::NoBrush);
        p->drawRoundedRect(rect.adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);
    }
    p->restore();
}

// Иконка + текст по центру, цветом fg. Наши SVG-иконки перекрашиваются под вариант кнопки.
void paintLabel(QPainter *p, const QRect &rect, const QIcon &icon, QSize iconSize, const QString &text,
                const QFont &font, const QColor &fg, qreal opacity, bool enabled)
{
    p->save();
    p->setOpacity(opacity);
    const QFontMetrics fm(font);
    QString shown = text;
    shown.remove(QLatin1Char('&'));
    const int textWidth = shown.isEmpty() ? 0 : fm.horizontalAdvance(shown);
    const int iconWidth = icon.isNull() ? 0 : iconSize.width();
    const int gap = (textWidth && iconWidth) ? spacing::s : 0;
    int x = rect.x() + (rect.width() - (iconWidth + gap + textWidth)) / 2;

    if (iconWidth) {
        const QString name = Icons::nameOf(icon);
        const QPixmap pm = name.isEmpty()
                               ? icon.pixmap(iconSize, dprOf(p), enabled ? QIcon::Normal : QIcon::Disabled)
                               : Icons::pixmap(name, iconSize.width(), fg, dprOf(p));
        const int y = rect.y() + (rect.height() - iconSize.height()) / 2;
        p->drawPixmap(QRect(x, y, iconSize.width(), iconSize.height()), pm);
        x += iconWidth + gap;
    }
    if (textWidth) {
        p->setFont(font);
        p->setPen(fg);
        p->drawText(QRect(x, rect.y(), textWidth + 1, rect.height()), Qt::AlignLeft | Qt::AlignVCenter, shown);
    }
    p->restore();
}

// --- Поверхности --------------------------------------------------------------

QColor surfaceColor(const QByteArray &surface)
{
    const ThemeTokens::Colors &c = T().color;
    if (surface == "panel")  return c.panel;
    if (surface == "panel2") return c.panel2;
    if (surface == "video")  return c.video;
    return c.bg;
}

void paintBorders(QPainter *p, const QRect &r, const QByteArray &borders)
{
    const QColor color = T().color.border;
    if (borders.contains("top"))    p->fillRect(QRect(r.left(), r.top(), r.width(), 1), color);
    if (borders.contains("bottom")) p->fillRect(QRect(r.left(), r.bottom(), r.width(), 1), color);
    if (borders.contains("left"))   p->fillRect(QRect(r.left(), r.top(), 1, r.height()), color);
    if (borders.contains("right"))  p->fillRect(QRect(r.right(), r.top(), 1, r.height()), color);
}

// --- Поля ---------------------------------------------------------------------

void paintField(QPainter *p, const QRectF &rect, const QWidget *widget, QStyle::State state, qreal radius)
{
    const ThemeTokens::Colors &c = T().color;
    const bool enabled = state & QStyle::State_Enabled;
    const bool focus = enabled && (state & QStyle::State_HasFocus);
    const bool hover = enabled && (state & QStyle::State_MouseOver);

    QColor border = focus ? c.accent : hover ? mix(c.border, c.textMuted, 0.35) : c.border;
    const QByteArray validation = prop(widget, "validation");
    const bool validated = validation == "ok" || validation == "warn" || validation == "error";
    if (validation == "ok")
        border = T().gradeColor(vs::Grade::Perfect);
    else if (validation == "warn")
        border = T().gradeColor(vs::Grade::Negative);
    else if (validation == "error")
        border = T().gradeColor(vs::Grade::Error);

    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    QRectF field = rect;
    if (focus && !validated) {
        // Ореол 3px accentSoft вокруг рамки.
        p->setPen(Qt::NoPen);
        p->setBrush(c.accentSoft);
        p->drawRoundedRect(field, radius + 3, radius + 3);
        field = field.adjusted(3, 3, -3, -3);
    }
    if (!enabled)
        p->setOpacity(0.6);
    p->setBrush(enabled ? c.inputBg : c.panel2);
    p->setPen(QPen(border, 1));
    p->drawRoundedRect(field.adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);
    p->restore();
}

qreal fieldRadius(const QWidget *widget)
{
    return prop(widget, "size") == "input" ? radius::input : radius::control;
}

bool isInnerEditor(const QWidget *widget)
{
    const QWidget *parent = widget ? widget->parentWidget() : nullptr;
    return qobject_cast<const QAbstractSpinBox *>(parent) || qobject_cast<const QComboBox *>(parent);
}

int buttonHeight(const QWidget *widget)
{
    const QByteArray segment = segmentOf(widget);
    if (segment == "modeTabs" || segment == "tabs") return metric::buttonHeight;
    if (segment == "pill")  return metric::themeToggleSegmentHeight;
    if (segment == "sets")  return metric::smallButtonHeight;
    if (segment == "chips") return 26;
    if (prop(widget, "role") == "badge") return 26;

    const QByteArray size = prop(widget, "size");
    if (size == "large") return metric::buttonHeightLarge;
    if (size == "small") return metric::smallButtonHeight;
    if (size == "input") return metric::commandInputHeight;
    return metric::buttonHeight;
}

int buttonWidth(const QWidget *widget, int textWidth, int iconWidth)
{
    const QByteArray segment = segmentOf(widget);
    const int gap = (textWidth && iconWidth) ? spacing::s : 0;
    if (segment == "pill")                        return metric::themeToggleSegmentWidth;
    if (segment == "sets")                        return qMax(34, textWidth + 2 * spacing::s);
    if (segment == "chips")                       return qMax(26, textWidth + 2 * spacing::m + 2);
    if (segment == "modeTabs" || segment == "tabs") return textWidth + iconWidth + gap + 2 * spacing::xl;
    if (prop(widget, "role") == "badge")          return iconWidth + gap + textWidth + 2 * 10;

    if (!textWidth) { // иконочная кнопка
        if (flag(widget, "wide"))
            return metric::playButtonWidth;
        const QByteArray size = prop(widget, "size");
        if (size == "input") return metric::commandInputHeight;
        if (size == "small") return 32;
        if (prop(widget, "variant") == "flat") return 24;
        return metric::iconButtonWidth;
    }
    return iconWidth + gap + textWidth + 2 * spacing::l;
}

QFont buttonFont(const QWidget *widget)
{
    const QByteArray segment = segmentOf(widget);
    if (segment == "modeTabs") return Fonts::modeTab();
    if (segment == "sets")     return Fonts::mono(font::small);
    if (!segment.isEmpty())    return Fonts::ui(font::small);

    const QByteArray variant = prop(widget, "variant");
    const bool large = prop(widget, "size") == "large";
    const int weight = (variant == "primary" || variant == "outline") ? 600 : 400;
    return Fonts::ui(large ? font::body : font::small, weight);
}

} // namespace

BeachStyle::BeachStyle(QStyle *base)
    : QProxyStyle(base)
{
}

void BeachStyle::polish(QPalette &palette)
{
    // Палитру задаёт ThemeManager целиком; здесь ничего не подмешиваем от базового стиля.
    Q_UNUSED(palette);
}

void BeachStyle::polish(QWidget *widget)
{
    QProxyStyle::polish(widget);
    const ThemeTokens::Colors &c = T().color;

    if (qobject_cast<QAbstractButton *>(widget) || qobject_cast<QLineEdit *>(widget)
        || qobject_cast<QComboBox *>(widget) || qobject_cast<QAbstractSpinBox *>(widget)
        || qobject_cast<QScrollBar *>(widget))
        widget->setAttribute(Qt::WA_Hover);

    // Поверхности, контейнеры сегментов и оверлей-чипы рисуют фон через PE_Widget.
    const QByteArray segmented = prop(widget, "segmented");
    const QByteArray role = prop(widget, "role");
    if (!prop(widget, "surface").isEmpty() || !segmented.isEmpty() || role == "overlayChip")
        widget->setAttribute(Qt::WA_StyledBackground);
    if (!segmented.isEmpty()) {
        QSizePolicy policy = widget->sizePolicy();
        policy.setVerticalPolicy(QSizePolicy::Fixed);
        widget->setSizePolicy(policy);
    }
    if (QLayout *layout = widget->layout(); layout && !segmented.isEmpty()) {
        if (segmented == "modeTabs" || segmented == "tabs") {
            layout->setContentsMargins(spacing::xs, spacing::xs, spacing::xs, spacing::xs);
            layout->setSpacing(spacing::xs);
        } else if (segmented == "pill") {
            layout->setContentsMargins(3, 3, 3, 3);
            layout->setSpacing(2);
        } else {
            layout->setSpacing(spacing::xs);
        }
    }

    if (widget->inherits("QTipLabel"))
        widget->setAttribute(Qt::WA_TranslucentBackground);

    auto setTextColor = [widget](const QColor &color) {
        QPalette pal = widget->palette();
        pal.setColor(QPalette::WindowText, color);
        widget->setPalette(pal);
    };

    if (auto *label = qobject_cast<QLabel *>(widget)) {
        const QByteArray validation = prop(widget, "validation");
        if (role == "sectionLabel") {
            label->setFont(Fonts::sectionLabel());
            setTextColor(c.textMuted);
        } else if (role == "muted") {
            label->setFont(Fonts::ui(font::small));
            setTextColor(c.textMuted);
        } else if (role == "mono") {
            label->setFont(Fonts::mono(font::body));
            setTextColor(c.text);
        } else if (role == "timecode") {
            label->setFont(Fonts::mono(font::caption));
            setTextColor(c.accent);
        } else if (role == "brand") {
            label->setFont(Fonts::ui(font::brand, 600));
            setTextColor(c.text);
        } else if (role == "overlayChip") {
            label->setFont(Fonts::mono(font::small));
            label->setContentsMargins(spacing::m, spacing::xs, spacing::m, spacing::xs);
            setTextColor(T().overlay.chipText);
        } else if (!validation.isEmpty()) {
            label->setFont(Fonts::ui(font::small));
            setTextColor(validation == "ok"     ? T().gradeColor(vs::Grade::Perfect)
                         : validation == "warn" ? T().gradeColor(vs::Grade::Negative)
                         : validation == "error" ? T().gradeColor(vs::Grade::Error)
                                                 : c.textMuted);
        }
    }

    if (auto *edit = qobject_cast<QLineEdit *>(widget); edit && !isInnerEditor(edit)) {
        if (prop(edit, "size") == "input")
            edit->setFont(Fonts::mono(font::command));
        else
            edit->setFont(flag(edit, "mono") ? Fonts::mono(font::input) : Fonts::ui(font::input));
        edit->setTextMargins(spacing::m, 0, spacing::m, 0);
    }
    if (auto *spin = qobject_cast<QAbstractSpinBox *>(widget))
        spin->setFont(flag(spin, "mono") ? Fonts::mono(font::input) : Fonts::ui(font::input));
    if (auto *combo = qobject_cast<QComboBox *>(widget))
        combo->setFont(Fonts::ui(font::small));
    if (qobject_cast<QPushButton *>(widget) || qobject_cast<QToolButton *>(widget))
        widget->setFont(buttonFont(widget));

    if (auto *view = qobject_cast<QAbstractItemView *>(widget)) {
        view->viewport()->setAttribute(Qt::WA_Hover);
        view->setMouseTracking(true);
        view->setFrameShape(QFrame::NoFrame);
        if (auto *table = qobject_cast<QTableView *>(view);
            table && !qobject_cast<QCalendarWidget *>(table->parentWidget())) {
            const bool sheet = role == "sheet";
            table->setShowGrid(sheet);
            table->verticalHeader()->setDefaultSectionSize(sheet ? metric::sheetRowHeight : metric::historyRowHeight);
            const QFont headerFont = sheet ? Fonts::ui(font::sheetSmall) : Fonts::ui(font::caption, 600);
            table->horizontalHeader()->setFont(headerFont);
            table->verticalHeader()->setFont(headerFont);
            if (sheet)
                table->verticalHeader()->setMinimumWidth(28); // гуттер с номерами строк, как в макете
            if (sheet)
                table->setFont(Fonts::mono(font::small));
        }
    }
}

void BeachStyle::unpolish(QWidget *widget)
{
    QProxyStyle::unpolish(widget);
}

int BeachStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    switch (metric) {
    case PM_ButtonIconSize:
    case PM_ToolBarIconSize:
        return metric::iconNormal;
    case PM_SmallIconSize:
        return metric::iconSmall;
    case PM_ScrollBarExtent:
        return metric::scrollBarWidth;
    case PM_ScrollBarSliderMin:
        return 28;
    case PM_DefaultFrameWidth:
    case PM_ComboBoxFrameWidth:
    case PM_SpinBoxFrameWidth:
        return 1;
    case PM_ButtonMargin:
        return spacing::l;
    case PM_ButtonDefaultIndicator:
    case PM_ButtonShiftHorizontal:
    case PM_ButtonShiftVertical:
        return 0;
    case PM_HeaderMargin:
        return spacing::xs;
    case PM_ToolTipLabelFrameWidth:
        return spacing::s;
    case PM_LayoutHorizontalSpacing:
    case PM_LayoutVerticalSpacing:
        return spacing::m;
    case PM_LayoutLeftMargin:
    case PM_LayoutRightMargin:
    case PM_LayoutTopMargin:
    case PM_LayoutBottomMargin:
        return spacing::l;
    default:
        return QProxyStyle::pixelMetric(metric, option, widget);
    }
}

int BeachStyle::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,
                          QStyleHintReturn *returnData) const
{
    switch (hint) {
    case SH_ScrollBar_Transient:
    case SH_EtchDisabledText:
    case SH_DitherDisabledText:
    case SH_UnderlineShortcut:
    case SH_ComboBox_Popup:
    case SH_Widget_Animation_Duration: // без анимаций и мигания фокуса
        return 0;
    case SH_ToolTip_WakeUpDelay:
        return 500;
    case SH_ItemView_ShowDecorationSelected:
        return 1;
    case SH_Table_GridLineColor:
        return int(T().sheet.gridLine.rgba());
    default:
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
}

QSize BeachStyle::sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size,
                                   const QWidget *widget) const
{
    switch (type) {
    case CT_PushButton:
        if (const auto *button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            const QFontMetrics fm(widget ? widget->font() : QApplication::font());
            QString text = button->text;
            text.remove(QLatin1Char('&'));
            const int textWidth = text.isEmpty() ? 0 : fm.horizontalAdvance(text);
            const int iconWidth = button->icon.isNull() ? 0 : button->iconSize.width();
            return {buttonWidth(widget, textWidth, iconWidth), buttonHeight(widget)};
        }
        break;
    case CT_ToolButton:
        if (const auto *button = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
            const QFontMetrics fm(widget ? widget->font() : QApplication::font());
            const bool showText = button->toolButtonStyle != Qt::ToolButtonIconOnly;
            const bool showIcon = button->toolButtonStyle != Qt::ToolButtonTextOnly && !button->icon.isNull();
            const int textWidth = showText && !button->text.isEmpty() ? fm.horizontalAdvance(button->text) : 0;
            const int iconWidth = showIcon ? button->iconSize.width() : 0;
            return {buttonWidth(widget, textWidth, iconWidth), buttonHeight(widget)};
        }
        break;
    case CT_LineEdit:
        if (!isInnerEditor(widget)) {
            const int height = prop(widget, "size") == "input" ? metric::commandInputHeight : metric::lineEditHeight;
            return {QProxyStyle::sizeFromContents(type, option, size, widget).width(), height};
        }
        break;
    case CT_ComboBox:
        return {QProxyStyle::sizeFromContents(type, option, size, widget).width() + spacing::m, 30};
    case CT_SpinBox:
        return {QProxyStyle::sizeFromContents(type, option, size, widget).width() + spacing::xl,
                metric::lineEditHeight};
    case CT_HeaderSection: {
        const QSize base = QProxyStyle::sizeFromContents(type, option, size, widget);
        return {base.width(), qMax(base.height(), 24)};
    }
    default:
        break;
    }
    return QProxyStyle::sizeFromContents(type, option, size, widget);
}

QRect BeachStyle::subControlRect(ComplexControl control, const QStyleOptionComplex *option, SubControl subControl,
                                 const QWidget *widget) const
{
    if (control == CC_ScrollBar) {
        const auto *bar = qstyleoption_cast<const QStyleOptionSlider *>(option);
        if (!bar)
            return QProxyStyle::subControlRect(control, option, subControl, widget);

        // Без стрелок: вся длина — жёлоб.
        const QRect r = bar->rect;
        const bool horizontal = bar->orientation == Qt::Horizontal;
        const int length = horizontal ? r.width() : r.height();
        const int range = bar->maximum - bar->minimum;
        int sliderLength = length;
        if (range > 0) {
            sliderLength = int(qint64(length) * bar->pageStep / (range + bar->pageStep));
            sliderLength = qBound(pixelMetric(PM_ScrollBarSliderMin, option, widget), sliderLength, length);
        }
        const int position = sliderPositionFromValue(bar->minimum, bar->maximum, bar->sliderPosition,
                                                     length - sliderLength, bar->upsideDown);
        auto span = [&](int from, int len) {
            return horizontal ? QRect(r.x() + from, r.y(), len, r.height()) : QRect(r.x(), r.y() + from, r.width(), len);
        };
        QRect result;
        switch (subControl) {
        case SC_ScrollBarGroove:  result = r; break;
        case SC_ScrollBarSlider:  result = span(position, sliderLength); break;
        case SC_ScrollBarSubPage: result = span(0, position); break;
        case SC_ScrollBarAddPage: result = span(position + sliderLength, length - position - sliderLength); break;
        default:                  return {};
        }
        return visualRect(bar->direction, r, result);
    }

    if (control == CC_ComboBox) {
        const QRect r = option->rect;
        const int arrowWidth = 28;
        switch (subControl) {
        case SC_ComboBoxArrow:
            return visualRect(option->direction, r, QRect(r.right() - arrowWidth + 1, r.y(), arrowWidth, r.height()));
        case SC_ComboBoxEditField:
            return visualRect(option->direction, r, r.adjusted(spacing::m, 1, -arrowWidth, -1));
        case SC_ComboBoxFrame:
            return r;
        default:
            break;
        }
    }
    return QProxyStyle::subControlRect(control, option, subControl, widget);
}

void BeachStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter,
                               const QWidget *widget) const
{
    const ThemeTokens::Colors &c = T().color;

    switch (element) {
    case PE_Widget: {
        const QByteArray surface = prop(widget, "surface");
        const QByteArray segmented = prop(widget, "segmented");
        if (!surface.isEmpty()) {
            painter->fillRect(option->rect, surfaceColor(surface));
            paintBorders(painter, option->rect, prop(widget, "borders"));
            return;
        }
        if (segmented == "modeTabs" || segmented == "tabs" || segmented == "pill") {
            ButtonLook look;
            look.bg = c.bg;
            look.border = c.border;
            look.radius = segmented == "pill" ? option->rect.height() / 2.0 : qreal(radius::input);
            paintPanel(painter, option->rect, look);
            return;
        }
        if (prop(widget, "role") == "overlayChip") {
            ButtonLook look;
            look.bg = T().overlay.chipBg;
            look.radius = radius::chip;
            paintPanel(painter, option->rect, look);
            return;
        }
        break;
    }

    case PE_PanelButtonCommand:
    case PE_PanelButtonBevel:
    case PE_PanelButtonTool: {
        const QStyle::State state = effectiveState(option->state, widget);
        paintPanel(painter, option->rect, buttonLook(widget, state, option->rect));
        return;
    }

    case PE_FrameFocusRect:
    case PE_FrameDefaultButton:
    case PE_FrameStatusBarItem:
        return; // фокус показывает рамка самого контрола

    case PE_PanelLineEdit: {
        const auto *frame = qstyleoption_cast<const QStyleOptionFrame *>(option);
        if ((frame && frame->lineWidth <= 0) || isInnerEditor(widget))
            return; // поле внутри комбобокса/даты — рамку рисует владелец
        paintField(painter, option->rect, widget, effectiveState(option->state, widget), fieldRadius(widget));
        return;
    }
    case PE_FrameLineEdit:
        return; // рамка — часть PE_PanelLineEdit

    case PE_IndicatorArrowDown:
    case PE_IndicatorArrowUp: {
        const int side = qMin(qMin(option->rect.width(), option->rect.height()), metric::iconSmall);
        const QRect target(option->rect.center().x() - side / 2, option->rect.center().y() - side / 2, side, side);
        const QColor color = (option->state & State_Enabled) ? c.textMuted : c.border;
        painter->drawPixmap(target, Icons::pixmap(element == PE_IndicatorArrowDown ? QStringLiteral("chevron-down")
                                                                                  : QStringLiteral("chevron-up"),
                                                  side, color, dprOf(painter)));
        return;
    }

    case PE_PanelTipLabel: {
        ButtonLook look;
        look.bg = c.text;
        look.radius = radius::control;
        paintPanel(painter, option->rect, look);
        return;
    }

    case PE_PanelMenu:
    case PE_FrameMenu: {
        painter->fillRect(option->rect, c.panel);
        painter->setPen(c.border);
        painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
        return;
    }

    case PE_PanelItemViewRow:
        return;

    case PE_PanelItemViewItem: {
        if (prop(widget, "role") == "sheet")
            return; // лист рисует делегат
        const auto *item = qstyleoption_cast<const QStyleOptionViewItem *>(option);
        const QRect r = option->rect;
        if (option->state & State_Selected) {
            painter->fillRect(r, c.accentSoft);
            int firstColumn = 0;
            if (const auto *table = qobject_cast<const QTableView *>(widget))
                firstColumn = table->horizontalHeader()->logicalIndexAt(0);
            if (item && item->index.column() == firstColumn)
                painter->fillRect(QRect(r.left(), r.top(), 3, r.height()), c.accent);
        } else if (option->state & State_MouseOver) {
            painter->fillRect(r, mix(c.panel2, c.border, 0.3));
        }
        if (qobject_cast<const QTableView *>(widget))
            painter->fillRect(QRect(r.left(), r.bottom(), r.width(), 1), c.rowDivider);
        return;
    }

    case PE_PanelScrollAreaCorner:
        painter->fillRect(option->rect, c.bg);
        return;

    default:
        break;
    }
    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

void BeachStyle::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter,
                             const QWidget *widget) const
{
    const ThemeTokens::Colors &c = T().color;

    switch (element) {
    case CE_PushButtonBevel:
        if (const auto *button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            const QStyle::State state = effectiveState(option->state, widget);
            const bool isDefault = button->features & QStyleOptionButton::DefaultButton;
            paintPanel(painter, option->rect, buttonLook(widget, state, option->rect, isDefault));
            return;
        }
        break;

    case CE_PushButtonLabel:
        if (const auto *button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            const QStyle::State state = effectiveState(option->state, widget);
            const bool isDefault = button->features & QStyleOptionButton::DefaultButton;
            const ButtonLook look = buttonLook(widget, state, option->rect, isDefault);
            QRect rect = option->rect;
            if (look.focusRing)
                rect.adjust(4, 4, -4, -4);
            paintLabel(painter, rect, button->icon, button->iconSize, button->text,
                       widget ? widget->font() : painter->font(), look.fg, look.opacity, state & State_Enabled);
            return;
        }
        break;

    case CE_HeaderSection: {
        const QRect r = option->rect;
        painter->fillRect(r, T().sheet.gutter);
        painter->fillRect(QRect(r.right(), r.top(), 1, r.height()), T().sheet.gridLine);
        painter->fillRect(QRect(r.left(), r.bottom(), r.width(), 1), T().sheet.gridLine);
        return;
    }
    case CE_HeaderLabel:
        if (const auto *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
            painter->save();
            painter->setPen(T().sheet.gutterText);
            painter->drawText(option->rect, int(header->textAlignment) | Qt::AlignVCenter, header->text);
            painter->restore();
            return;
        }
        break;
    case CE_HeaderEmptyArea:
        painter->fillRect(option->rect, T().sheet.gutter);
        return;

    case CE_ItemViewItem:
        if (const auto *item = qstyleoption_cast<const QStyleOptionViewItem *>(option)) {
            // Выделение — мягкий accentSoft, поэтому текст остаётся цвета text, а не onAccent.
            QStyleOptionViewItem copy(*item);
            copy.palette.setColor(QPalette::HighlightedText, c.text);
            QProxyStyle::drawControl(element, &copy, painter, widget);
            return;
        }
        break;

    default:
        break;
    }
    QProxyStyle::drawControl(element, option, painter, widget);
}

void BeachStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter,
                                    const QWidget *widget) const
{
    const ThemeTokens::Colors &c = T().color;

    switch (control) {
    case CC_ToolButton:
        if (const auto *button = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
            const QStyle::State state = effectiveState(option->state, widget);
            const ButtonLook look = buttonLook(widget, state, option->rect);
            paintPanel(painter, option->rect, look);
            const bool showText = button->toolButtonStyle != Qt::ToolButtonIconOnly;
            const bool showIcon = button->toolButtonStyle != Qt::ToolButtonTextOnly;
            paintLabel(painter, option->rect, showIcon ? button->icon : QIcon(), button->iconSize,
                       showText ? button->text : QString(), widget ? widget->font() : painter->font(), look.fg,
                       look.opacity, state & State_Enabled);
            return;
        }
        break;

    case CC_ComboBox:
        if (const auto *combo = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            const QStyle::State state = effectiveState(option->state, widget);
            paintField(painter, option->rect, widget, state, radius::control);
            const bool dateEdit = qobject_cast<const QDateTimeEdit *>(widget);
            const QRect arrow = subControlRect(control, option, SC_ComboBoxArrow, widget);
            const int side = metric::iconSmall;
            const QRect target(arrow.center().x() - side / 2, arrow.center().y() - side / 2, side, side);
            painter->drawPixmap(target, Icons::pixmap(dateEdit ? QStringLiteral("calendar") : QStringLiteral("chevron-down"),
                                                      side, c.textMuted, dprOf(painter)));
            Q_UNUSED(combo);
            return;
        }
        break;

    case CC_SpinBox:
        if (const auto *spin = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            paintField(painter, option->rect, widget, effectiveState(option->state, widget), radius::control);
            if (spin->buttonSymbols != QAbstractSpinBox::NoButtons) {
                QStyleOption arrow(*option);
                arrow.rect = subControlRect(control, option, SC_SpinBoxUp, widget);
                drawPrimitive(PE_IndicatorArrowUp, &arrow, painter, widget);
                arrow.rect = subControlRect(control, option, SC_SpinBoxDown, widget);
                drawPrimitive(PE_IndicatorArrowDown, &arrow, painter, widget);
            }
            return;
        }
        break;

    case CC_ScrollBar:
        if (const auto *bar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            const QStyle::State state = effectiveState(option->state, widget);
            const bool active = (state & State_MouseOver) || (state & State_Sunken);
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->fillRect(option->rect, active ? c.panel2 : c.bg);
            const QRect slider = subControlRect(control, option, SC_ScrollBarSlider, widget);
            const qreal inset = active ? 1 : 2;
            const QRectF knob = QRectF(slider).adjusted(inset, inset, -inset, -inset);
            const qreal radius = active ? 4 : 3;
            painter->setPen(Qt::NoPen);
            painter->setBrush(active ? c.textMuted : c.border);
            painter->drawRoundedRect(knob, radius, radius);
            painter->restore();
            Q_UNUSED(bar);
            return;
        }
        break;

    default:
        break;
    }
    QProxyStyle::drawComplexControl(control, option, painter, widget);
}

QIcon BeachStyle::standardIcon(StandardPixmap standardIcon, const QStyleOption *option, const QWidget *widget) const
{
    switch (standardIcon) {
    case SP_DialogCloseButton:
    case SP_TitleBarCloseButton:
    case SP_DialogCancelButton:  return Icons::get(QStringLiteral("close"));
    case SP_MediaPlay:           return Icons::get(QStringLiteral("play"));
    case SP_MediaPause:          return Icons::get(QStringLiteral("pause"));
    case SP_MediaSeekBackward:   return Icons::get(QStringLiteral("seek-back"));
    case SP_MediaSeekForward:    return Icons::get(QStringLiteral("seek-forward"));
    case SP_DirOpenIcon:
    case SP_DirIcon:
    case SP_DirClosedIcon:       return Icons::get(QStringLiteral("folder-open"), Icons::Role::Muted);
    case SP_FileIcon:            return Icons::get(QStringLiteral("file"), Icons::Role::Muted);
    case SP_DialogOkButton:
    case SP_DialogApplyButton:   return Icons::get(QStringLiteral("check"));
    case SP_DialogSaveButton:    return Icons::get(QStringLiteral("export"));
    case SP_MessageBoxWarning:
    case SP_MessageBoxCritical:  return Icons::get(QStringLiteral("alert"), Icons::Role::Danger);
    case SP_MessageBoxInformation:
    case SP_MessageBoxQuestion:  return Icons::get(QStringLiteral("alert"), Icons::Role::Accent);
    case SP_ArrowDown:           return Icons::get(QStringLiteral("chevron-down"), Icons::Role::Muted);
    case SP_ArrowUp:             return Icons::get(QStringLiteral("chevron-up"), Icons::Role::Muted);
    default:
        return QProxyStyle::standardIcon(standardIcon, option, widget);
    }
}
