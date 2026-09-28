#include "theme/fonts.h"
#include "theme/themetokens.h"

#include <QFontDatabase>

namespace Fonts {

namespace {
const QString kUiFamily = QStringLiteral("IBM Plex Sans");
const QString kMonoFamily = QStringLiteral("JetBrains Mono");

QFont make(const QString &family, int pixelSize, int weight)
{
    QFont font(family);
    font.setPixelSize(pixelSize);
    font.setWeight(QFont::Weight(weight));
    font.setHintingPreference(QFont::PreferNoHinting);
    return font;
}
} // namespace

void registerAll()
{
    for (const char *file : {"IBMPlexSans-Regular.ttf", "IBMPlexSans-Medium.ttf", "IBMPlexSans-SemiBold.ttf",
                             "JetBrainsMono-Regular.ttf", "JetBrainsMono-SemiBold.ttf"}) {
        QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/") + QLatin1String(file));
    }
}

QFont ui(int pixelSize, int weight)
{
    return make(kUiFamily, pixelSize, weight);
}

QFont mono(int pixelSize, int weight)
{
    QFont font = make(kMonoFamily, pixelSize, weight);
    font.setStyleHint(QFont::Monospace);
    return font;
}

QFont sectionLabel()
{
    QFont font = ui(tokens::font::sectionLabelSize, tokens::font::sectionLabelWeight);
    font.setCapitalization(QFont::AllUppercase);
    font.setLetterSpacing(QFont::PercentageSpacing, tokens::font::sectionLabelSpacing);
    return font;
}

QFont modeTab()
{
    QFont font = ui(tokens::font::modeTabSize, tokens::font::modeTabWeight);
    font.setCapitalization(QFont::AllUppercase);
    font.setLetterSpacing(QFont::PercentageSpacing, tokens::font::modeTabSpacing);
    return font;
}

} // namespace Fonts
