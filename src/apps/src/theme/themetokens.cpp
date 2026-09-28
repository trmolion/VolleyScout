#include "theme/themetokens.h"

namespace {

QColor hex(const char *value)
{
    return QColor(QLatin1String(value));
}

ThemeTokens::Overlay makeOverlay()
{
    // rgba(0,0,0,0.60) · #FFFFFF · #D6D0C4 · rgba(255,255,255,0.25)
    return {QColor(0, 0, 0, 153), hex("#FFFFFF"), hex("#D6D0C4"), QColor(255, 255, 255, 64)};
}

} // namespace

const ThemeTokens &ThemeTokens::day()
{
    static const ThemeTokens tokens{
        QStringLiteral("Полдень"),
        {
            hex("#F5ECDA"), hex("#FFF9EE"), hex("#FBF3E4"), hex("#E3D3B5"), hex("#EFE3CC"),
            hex("#2B2118"), hex("#6F6152"),
            hex("#0B7F80"), hex("#FFFFFF"), hex("#DCEEEA"), hex("#0B6FA8"),
            hex("#FFFFFF"), hex("#1A1712"), hex("#E9D6C4"), hex("#B53532"), hex("#8A7B69"),
        },
        {hex("#1E7D45"), hex("#557F1B"), hex("#8A6D00"), hex("#A84E17"), hex("#B53532")},
        {
            hex("#FFFFFF"), hex("#FBF7EF"), hex("#F3EBDC"), hex("#EBE1CD"), hex("#6F6152"),
            hex("#E3D8C4"), hex("#A8977A"), hex("#6F6152"), hex("#E3F2EF"), hex("#0B7F80"),
        },
        makeOverlay(),
    };
    return tokens;
}

const ThemeTokens &ThemeTokens::night()
{
    static const ThemeTokens tokens{
        QStringLiteral("Сумерки"),
        {
            hex("#1A1611"), hex("#231D16"), hex("#1E1913"), hex("#3A3024"), hex("#2C251C"),
            hex("#F3E9D8"), hex("#B5A58E"),
            hex("#2FB5B3"), hex("#06201F"), hex("#17302D"), hex("#7CC4EC"),
            hex("#15120E"), hex("#0D0B08"), hex("#3A2A22"), hex("#EF6461"), hex("#7F705E"),
        },
        {hex("#5FD08A"), hex("#A6D96A"), hex("#F2D14B"), hex("#F0955A"), hex("#EF6461")},
        {
            hex("#211B15"), hex("#1D1812"), hex("#2C251C"), hex("#171310"), hex("#9A8B76"),
            hex("#352C21"), hex("#5E5040"), hex("#B5A58E"), hex("#173330"), hex("#2FB5B3"),
        },
        makeOverlay(),
    };
    return tokens;
}
