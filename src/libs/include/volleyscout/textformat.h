#pragma once

#include <QString>
#include <QtGlobal>

namespace vs {

// Время матча в формате «h:mm:ss».
QString formatTime(qint64 ms);

// Число с существительным в нужной форме: «1 запись», «3 записи», «14 записей».
QString pluralRu(qint64 n, const QString &one, const QString &few, const QString &many);

} // namespace vs
