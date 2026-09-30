#pragma once

#include <QString>

// --selftest <dir> [video]: проверка собранного/развёрнутого приложения без участия человека
// (под Wine и на чистой Windows). Проверяет плагины Qt (платформа, SVG, FFmpeg), шрифты,
// тему, выгрузку .xlsx и архива скаутов теми же функциями, что и кнопки «Сформировать…»,
// воспроизведение видео и рендер окна в обеих темах (<dir>/screens/*.png).
// Итог — в <dir>/selftest.txt и stdout; код выхода — число проваленных проверок.
int runSelfTest(const QString &directory, const QString &videoPath);
