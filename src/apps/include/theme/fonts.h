#pragma once

#include <QFont>

// IBM Plex Sans (интерфейс) и JetBrains Mono (таймкоды, команды, числа) из ресурсов.
namespace Fonts {

void registerAll();

QFont ui(int pixelSize, int weight = 400);
QFont mono(int pixelSize, int weight = 400);

// «ИСТОРИЯ», «ПАРТИЯ»: 11px, 600, ВЕРХНИЙ РЕГИСТР, разрядка 110%.
QFont sectionLabel();
// «ФОРМИРОВАНИЕ | ТАБЛИЦА»: 12px, 600, ВЕРХНИЙ РЕГИСТР, разрядка 108%.
QFont modeTab();

} // namespace Fonts
