#!/usr/bin/env bash
# Проверка инсталлятора под Wine в ЧИСТОМ префиксе (build-win/wineprefix-install, пересоздаётся):
# тихая установка → файлы, ярлыки, запись в «Установка и удаление программ» → самопроверка
# установленного приложения → тихое удаление → не осталось файлов, ярлыков и записи в реестре.
#
#   scripts/test-installer.sh [путь к VolleyScout-<версия>-win64.exe]
set -euo pipefail
source "$(dirname "$0")/windows-env.sh"
require_tools wine winepath xvfb-run

installer="${1:-$(ls -1t "$DIST_DIR"/VolleyScout-*-win64.exe 2>/dev/null | head -1)}"
[[ -f "$installer" ]] || fail "не найден инсталлятор (dist/VolleyScout-*-win64.exe)"

export WINEPREFIX="$BUILD_DIR/wineprefix-install"
export WINEDEBUG=-all WINEDLLOVERRIDES="mscoree,mshtml="
step "Чистый Wine-префикс $WINEPREFIX"
wineserver -k 2>/dev/null || true
rm -rf "$WINEPREFIX"
mkdir -p "$WINEPREFIX"
wineboot --init >/dev/null 2>&1
wineserver --wait

install_dir="$WINEPREFIX/drive_c/Program Files/VolleyScout"
# Заглушки NSIS 32-битные — запись об установке в 32-битном представлении реестра.
uninstall_key='HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\VolleyScout'
reg_query() { wine reg query "$uninstall_key" /reg:32 2>/dev/null | tr -d '\r'; }
# `wine reg` пишет «ключ не найден» в stdout — наличие ключа определяем по коду возврата.
reg_exists() { wine reg query "$uninstall_key" /reg:32 >/dev/null 2>&1; }
errors=0
check() { if eval "$2"; then info "✓ $1"; else printf '\033[1;31m    ✗ %s\033[0m\n' "$1"; ((++errors)); fi; }
find_links() { find "$WINEPREFIX/drive_c/users" "$WINEPREFIX/drive_c/ProgramData" -iname 'VolleyScout*.lnk' 2>/dev/null; }

step "Установка: $(basename "$installer") /S"
# Виртуальный дисплей живёт, пока не завершатся ВСЕ процессы Wine (wineserver --wait внутри).
in_xvfb() { xvfb-run -a bash -c "$1; wineserver --wait"; }
in_xvfb "wine '$(winepath -w "$installer")' /S" >/dev/null 2>&1
check "установлен VolleyScout.exe в Program Files" "[[ -f '$install_dir/VolleyScout.exe' ]]"
check "установлен деинсталлятор" "[[ -f '$install_dir/Uninstall.exe' ]]"
check "установлено столько же файлов, сколько в deploy/" \
    "[[ \$(find '$install_dir' -type f ! -name Uninstall.exe | wc -l) -eq \$(find '$DEPLOY_DIR' -type f | wc -l) ]]"
links="$(find_links)"
printf '%s\n' "$links" | sed 's|^|      ярлык: |'
check "ярлык в «Пуске»" "grep -qi 'Start Menu/Programs/VolleyScout/VolleyScout.lnk' <<<\"\$links\""
check "ярлык на рабочем столе" "grep -qi 'Desktop/VolleyScout.lnk' <<<\"\$links\""
# Ярлыки указывают на установленный exe (а не, например, на несуществующий bin\VolleyScout.exe).
while IFS= read -r link; do
    [[ -n "$link" ]] || continue
    # Путь в .lnk бывает и в UTF-16, и в 8-битной кодировке — смотрим оба варианта.
    target="$({ strings -a -el "$link"; strings -a "$link"; } | grep -i 'VolleyScout\\VolleyScout.exe' | head -1 || true)"
    check "ярлык $(basename "$(dirname "$link")")/$(basename "$link") → $target" \
        "[[ '$target' == *'Program Files\\VolleyScout\\VolleyScout.exe' || '$target' == *'Program Files\\VolleyScout\\Uninstall.exe' ]]"
done <<<"$links"

reg="$(reg_query || true)"
printf '%s\n' "$reg" | grep -E 'Display|Publisher|Uninstall' | sed 's/^\s*/      /' || true
check "запись «Установка и удаление программ»: имя" "grep -q 'DisplayName.*VolleyScout' <<<\"\$reg\""
check "запись: версия" "grep -q 'DisplayVersion' <<<\"\$reg\""
check "запись: издатель" "grep -q 'Publisher.*VolleyScout' <<<\"\$reg\""
check "запись: иконка" "grep -q 'DisplayIcon.*VolleyScout.exe' <<<\"\$reg\""

step "Самопроверка установленного приложения"
if "$PROJECT_ROOT/scripts/test-windows.sh" "$install_dir"; then info "✓ приложение из Program Files работает"
else printf '\033[1;31m    ✗ приложение из Program Files\033[0m\n'; ((++errors)); fi
export WINEPREFIX="$BUILD_DIR/wineprefix-install" # test-windows.sh работает в своём префиксе

step "Удаление: Uninstall.exe /S"
# Деинсталлятор NSIS копирует себя во временную папку и перезапускается оттуда: первый
# процесс сразу завершается, поэтому дисплей держим до конца всех процессов Wine.
in_xvfb "wine '$(winepath -w "$install_dir/Uninstall.exe")' /S" >/dev/null 2>&1
check "папка программы удалена" "[[ ! -e '$install_dir' ]]"
check "ярлыки удалены" "[[ -z \"\$(find_links)\" ]]"
check "запись в реестре удалена" "! reg_exists"

echo
((errors == 0)) || fail "проверка инсталлятора не пройдена: ошибок $errors"
printf '\033[1;32mПроверка инсталлятора пройдена\033[0m\n'
