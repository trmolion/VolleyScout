#pragma once

#include <QString>

// --screenshots <dir>: главное окно (1440×900, оба режима) и StyleGallery в обеих темах
// рендерятся через QWidget::grab() в PNG, затем приложение завершается.
// Демо-данные — как в mockups (записи истории и скаут на 40 действий).
int runScreenshots(const QString &directory);
