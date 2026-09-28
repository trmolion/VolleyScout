#include "volleyscout/scoutentry.h"

namespace vs {

namespace {

// Символы, недопустимые в именах файлов Windows/Linux, заменяются на «_».
QString sanitized(QString text)
{
    static const QString forbidden = QStringLiteral("\\/:*?\"<>|");
    for (QChar &ch : text) {
        if (forbidden.contains(ch) || ch.unicode() < 0x20)
            ch = u'_';
    }
    return text.trimmed();
}

} // namespace

QString MatchInfo::baseName() const
{
    return sanitized(QStringLiteral("%1_%2 vs %3")
                         .arg(date.toString(QStringLiteral("dd.MM.yyyy")), team1.trimmed(),
                              team2.trimmed()));
}

} // namespace vs
