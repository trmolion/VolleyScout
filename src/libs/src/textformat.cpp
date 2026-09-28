#include "volleyscout/textformat.h"

namespace vs {

QString formatTime(qint64 ms)
{
    const qint64 totalSeconds = qMax<qint64>(0, ms) / 1000;
    const qint64 hours = totalSeconds / 3600;
    const qint64 minutes = (totalSeconds % 3600) / 60;
    const qint64 seconds = totalSeconds % 60;
    return QStringLiteral("%1:%2:%3")
        .arg(hours)
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
}

} // namespace vs

namespace vs {

QString pluralRu(qint64 n, const QString &one, const QString &few, const QString &many)
{
    const qint64 mod100 = qAbs(n) % 100;
    const qint64 mod10 = mod100 % 10;
    const QString &word = (mod100 >= 11 && mod100 <= 14) ? many
                          : mod10 == 1                   ? one
                          : (mod10 >= 2 && mod10 <= 4)   ? few
                                                         : many;
    return QString::number(n) + u' ' + word;
}

} // namespace vs
