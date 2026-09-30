#pragma once

#include <QScrollArea>

// Лист состояний контролов (как mockups/png/components_*.png) — для проверки BeachStyle.
// Открывается по --gallery; hover/pressed/focus показаны принудительно (свойство forceState).
class StyleGallery : public QScrollArea
{
    Q_OBJECT

public:
    explicit StyleGallery(QWidget *parent = nullptr);

    // Всё содержимое целиком — для скриншота.
    QWidget *content() const { return widget(); }
};
