#pragma once

#include <QProxyStyle>

// Оформление «Пляж» поверх Fusion. Все цвета — из ThemeManager::tokens().
//
// Варианты задаются динамическими свойствами виджетов:
//   variant     primary | outline | secondary | ghost | flat | overlay   — кнопки
//   size        normal (32) | large (40) | small (28) | input (52)        — кнопки, поле команды
//   wide        true — кнопка «плей» шириной playButtonWidth
//   iconRole    muted | accent | danger — цвет иконки вместо цвета варианта
//   danger      true — flat-кнопка удаления: на hover иконка danger
//   validation  none | ok | warn | error                                   — поле команды и подсказка
//   role        sectionLabel | muted | mono | brand | overlayChip | badge | sheet
//   mono        true — моноширинный шрифт у поля ввода / даты
//   segmented   modeTabs | tabs | pill | sets | chips   — контейнер группы кнопок
//   surface     bg | panel | panel2 | video; borders — «top bottom left right»
//   forceState  hover | pressed | focus — только для галереи/скриншотов
class BeachStyle : public QProxyStyle
{
    Q_OBJECT

public:
    explicit BeachStyle(QStyle *base);

    void polish(QPalette &palette) override;
    void polish(QWidget *widget) override;
    void unpolish(QWidget *widget) override;

    int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr,
                    const QWidget *widget = nullptr) const override;
    int styleHint(StyleHint hint, const QStyleOption *option = nullptr, const QWidget *widget = nullptr,
                  QStyleHintReturn *returnData = nullptr) const override;
    QSize sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size,
                           const QWidget *widget) const override;
    QRect subControlRect(ComplexControl control, const QStyleOptionComplex *option, SubControl subControl,
                         const QWidget *widget) const override;

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter,
                       const QWidget *widget = nullptr) const override;
    void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter,
                     const QWidget *widget = nullptr) const override;
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter,
                            const QWidget *widget = nullptr) const override;

    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *option = nullptr,
                       const QWidget *widget = nullptr) const override;
};
