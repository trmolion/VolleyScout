# Сборка VolleyScout

C++20, Qt 6, CMake.

## Сборка под Linux

```bash
cmake -B build && cmake --build build
./build/bin/VolleyScout
```

Нужны Qt 6.5+ (модули Widgets, Svg, Multimedia) и сеть при первой конфигурации — CMake скачивает QXlsx.

Служебные режимы:

| Ключ | Что делает |
|---|---|
| `--gallery` | окно со всеми состояниями контролов темы |
| `--screenshots <папка>` | PNG главного окна (оба режима) и галереи в обеих темах |
| `--selftest <папка> <видео>` | самопроверка: плагины Qt, шрифты, тема, `.xlsx` и архив скаутов, воспроизведение видео, скриншоты; итог в `<папка>/selftest.txt`, код выхода — число провалов |

## Сборка под Windows (кросс-сборка на Arch Linux)

Windows-версия собирается на Linux: [quasi-msys2](https://github.com/HolyBlackCat/quasi-msys2) (окружение UCRT64) + системный clang/lld, деплой — `windeployqt6.exe` под Wine, инсталлятор — CPack + NSIS.

### Зависимости Arch

```bash
sudo pacman -S --needed clang lld llvm wine make wget tar zstd gawk gnupg ninja cmake \
    imagemagick xorg-server-xvfb ffmpeg unzip python
yay -S nsis                     # makensis (AUR)
```

### Первый запуск: quasi-msys2

По умолчанию ставится в `~/.local/share/quasi-msys2` (путь задаётся переменной `QUASI_MSYS2_DIR`). В репозиторий он не входит.

```bash
git clone https://github.com/HolyBlackCat/quasi-msys2 ~/.local/share/quasi-msys2
cd ~/.local/share/quasi-msys2
echo UCRT64 > msystem.txt
make install _gcc _qt6-base _qt6-svg _qt6-multimedia _qt6-tools _ntldd
```

`_qt6-multimedia` подтягивает `qt6-multimedia-ffmpeg` и FFmpeg. Обновление пакетов: `make upgrade`.

### Команды

```bash
scripts/package-windows.sh      # всё сразу → dist/VolleyScout-<версия>-win64.exe и .zip
```

Этапы по отдельности:

| Скрипт | Что делает |
|---|---|
| `scripts/build-windows.sh` | Release-сборка в `build-win/` → `build-win/bin/VolleyScout.exe` |
| `scripts/deploy-windows.sh` | `build-win/deploy/`: exe + Qt (windeployqt6 под Wine) + все транзитивные DLL из MSYS2 |
| `scripts/check-deploy.sh` | проверка `deploy/`: зависимости всех .exe/.dll (рекурсивно), обязательные плагины, мусор, размер |
| `scripts/test-windows.sh [папка]` | запуск `--selftest` под Wine (префикс `build-win/wineprefix`, невидимый дисплей) |
| `scripts/test-installer.sh [инсталлятор]` | чистый префикс: установка → самопроверка → удаление без остатков |
| `scripts/generate-icons.sh` | `app.ico` из `app-logo.svg` (нужен только при смене логотипа) |

`package-windows.sh` останавливается на первой проваленной проверке — пакет не собирается. `SKIP_WINE_TESTS=1` пропускает проверки запуском под Wine (деплой проверяется всегда).

Wine запускается только в префиксах внутри `build-win/` — пользовательский `~/.wine` не используется.

### Что проверить на настоящей Windows

Под Wine проверяется всё, кроме того, что зависит от настоящей системы:

1. Установка с интерфейсом (без `/S`): страницы на русском, лицензия, выбор папки, «Запустить VolleyScout» в конце.
2. SmartScreen: инсталлятор не подписан — «Подробнее → Выполнить в любом случае».
3. Видео: воспроизведение со звуком, плавность на 1080p/50 fps, перемотка по таймлайну, пробел — старт/пауза.
4. Кнопки «Сформировать таблицу» и «Сформировать скауты» через системный диалог сохранения, открытие `.xlsx` в Excel.
5. Иконки: exe в Проводнике, ярлыки, панель задач, «Установка и удаление программ»; свойства файла → «Подробно» (описание, версия).
6. Переключение темы, масштаб экрана 125–150 %.
7. Удаление через «Установка и удаление программ»: не остаётся папки и ярлыков.
