#!/usr/bin/env bash
# Одна команда для релиза под Windows:
#   сборка → деплой → check-deploy.sh → запуск под Wine → CPack (NSIS + ZIP) → проверка инсталлятора.
# Любая неудачная проверка останавливает скрипт, пакет не собирается.
# Результат: dist/VolleyScout-<версия>-win64.exe и dist/VolleyScout-<версия>-win64.zip.
#
# SKIP_WINE_TESTS=1 — пропустить проверки запуском под Wine (деплой всё равно проверяется).
set -euo pipefail
source "$(dirname "$0")/windows-env.sh"
require_tools cpack makensis

"$PROJECT_ROOT/scripts/build-windows.sh"
"$PROJECT_ROOT/scripts/deploy-windows.sh"
"$PROJECT_ROOT/scripts/check-deploy.sh"
if [[ "${SKIP_WINE_TESTS:-0}" != 1 ]]; then
    "$PROJECT_ROOT/scripts/test-windows.sh"
fi

step "CPack: NSIS + ZIP"
packages="$BUILD_DIR/packages"
rm -rf "$packages"
cpack --config "$BUILD_DIR/CPackConfig.cmake" -B "$packages" || fail "cpack"
mkdir -p "$DIST_DIR"
shopt -s nullglob
built=("$packages"/VolleyScout-*-win64.exe "$packages"/VolleyScout-*-win64.zip)
((${#built[@]} == 2)) || fail "cpack не создал .exe и .zip (см. $packages)"
cp -f "${built[@]}" "$DIST_DIR/"

if [[ "${SKIP_WINE_TESTS:-0}" != 1 ]]; then
    "$PROJECT_ROOT/scripts/test-installer.sh" "$DIST_DIR/$(basename "${built[0]}")"
fi

step "Готово"
for file in "${built[@]}"; do
    info "$DIST_DIR/$(basename "$file")  ($(du -h "$file" | cut -f1))"
done
