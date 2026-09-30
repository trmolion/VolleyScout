#pragma once

#include "volleyscout/command.h"

#include <QColor>
#include <QString>

#include <array>

// Токены гаммы «Пляж» — 1:1 из design/tokens.json комплекта оформления.
//
// Значения захардкожены, а не читаются из JSON в рантайме: опечатка в имени токена
// ловится компилятором, а не превращается в тихий чёрный цвет; тема не может
// «не загрузиться». Цена — при правке tokens.json этот файл обновляется вручную.
//
// Это единственное место в проекте, где цвета записаны литералами.
struct ThemeTokens
{
    QString name;

    struct Colors
    {
        QColor bg, panel, panel2, border, rowDivider;
        QColor text, textMuted;
        QColor accent, onAccent, accentSoft, link;
        QColor inputBg, video, trimCut, danger, placeholder;
    } color;

    std::array<QColor, vs::kAllGrades.size()> grade; // индекс — int(vs::Grade): # + ! - =

    struct Sheet
    {
        QColor cell, cellAlt, header, gutter, gutterText;
        QColor gridLine, groupLine, percentText, rowHighlight, selection;
    } sheet;

    // Поверх видео — одинаково в обеих темах.
    struct Overlay
    {
        QColor chipBg, chipText, chipTextMuted, chipBorder;
    } overlay;

    QColor gradeColor(vs::Grade grade) const { return this->grade[std::size_t(grade)]; }

    static const ThemeTokens &day();
    static const ThemeTokens &night();
};

// Размеры, радиусы, отступы и типографика — общие для обеих тем.
namespace tokens {

namespace radius {
inline constexpr int chip = 4;
inline constexpr int control = 6;
inline constexpr int input = 8;
inline constexpr int segment = 5;
inline constexpr int pill = 999;
} // namespace radius

namespace spacing {
inline constexpr int xs = 4;
inline constexpr int s = 6;
inline constexpr int m = 8;
inline constexpr int l = 12;
inline constexpr int xl = 16;
inline constexpr int xxl = 20;
inline constexpr int section = 24;
} // namespace spacing

namespace metric {
inline constexpr int headerHeight = 56;
inline constexpr int leftPaneStretch = 4; // leftPaneFraction 0.8 → 4 : 1
inline constexpr int rightPaneStretch = 1;
inline constexpr int rightPaneMinWidth = 260;
inline constexpr int transportHeight = 48;
inline constexpr int timelineBlockHeight = 88;
inline constexpr int timelineBarHeight = 28;
inline constexpr int trimBarHeight = 10;
inline constexpr int buttonHeight = 32;
inline constexpr int buttonHeightLarge = 40;
inline constexpr int smallButtonHeight = 28;
inline constexpr int iconButtonWidth = 36;
inline constexpr int playButtonWidth = 44;
inline constexpr int lineEditHeight = 38;
inline constexpr int commandInputHeight = 52;
inline constexpr int historyRowHeight = 36;
inline constexpr int sheetRowHeight = 26;
inline constexpr int themeToggleSegmentWidth = 34;
inline constexpr int themeToggleSegmentHeight = 30;
inline constexpr int iconSmall = 14;
inline constexpr int iconNormal = 16;
inline constexpr int iconLarge = 18;
inline constexpr int focusRingWidth = 2;
inline constexpr int scrollBarWidth = 10;
} // namespace metric

namespace font {
inline constexpr int caption = 11;
inline constexpr int small = 12;
inline constexpr int body = 13;
inline constexpr int input = 14;
inline constexpr int brand = 15;
inline constexpr int command = 18;
inline constexpr int videoTimecode = 20;
inline constexpr int sheetSmall = 10; // проценты листа, заголовки строк/колонок
inline constexpr int sectionLabelSize = 11;
inline constexpr int sectionLabelWeight = 600;
inline constexpr int sectionLabelSpacing = 110; // %
inline constexpr int modeTabSize = 12;
inline constexpr int modeTabWeight = 600;
inline constexpr int modeTabSpacing = 108; // %
} // namespace font

} // namespace tokens
