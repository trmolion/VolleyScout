#!/usr/bin/env bash
# Развёртывание Windows-сборки в build-win/deploy/: exe + Qt (windeployqt6 под Wine)
# + все транзитивные DLL из MSYS2 (FFmpeg и его кодеки, рантайм компилятора и т.д.).
# Папка должна запускаться на чистой Windows; проверка — scripts/check-deploy.sh.
set -euo pipefail
source "$(dirname "$0")/windows-env.sh"
source "$(dirname "$0")/pe-imports.sh"

require_tools wine winepath llvm-readobj
enter_msys2_env

exe="$BUILD_DIR/bin/VolleyScout.exe"
[[ -f "$exe" ]] || fail "нет $exe — сначала scripts/build-windows.sh"

step "Чистая папка $DEPLOY_DIR"
rm -rf "$DEPLOY_DIR"
mkdir -p "$DEPLOY_DIR"
cp "$exe" "$DEPLOY_DIR/"

step "windeployqt6 (Wine)"
# Рантайм компилятора кладём сами ниже — по реальной таблице импортов, а не по догадке windeployqt.
log="$BUILD_DIR/windeployqt.log"
WINEPATH="$(winepath -w "$MSYSTEM_PREFIX/bin")" wine "$MSYSTEM_PREFIX/bin/windeployqt6.exe" \
    --qtpaths "$(winepath -w "$MSYSTEM_PREFIX/bin/qtpaths6.exe")" \
    --release \
    --no-translations \
    --no-compiler-runtime \
    --no-opengl-sw \
    --no-system-d3d-compiler \
    --no-system-dxc-compiler \
    --verbose 1 \
    --dir "$(winepath -w "$DEPLOY_DIR")" \
    "$(winepath -w "$DEPLOY_DIR/VolleyScout.exe")" >"$log" 2>&1 || { tail -20 "$log"; fail "windeployqt6 (лог: $log)"; }
wineserver --wait
info "лог: $log"

step "Транзитивные зависимости из MSYS2"
# windeployqt знает про Qt, но не про зависимости сторонних DLL (кодеки FFmpeg, zlib, ICU, …).
load_system_dlls
declare -A MSYS2_DLLS
while IFS= read -r dll; do
    MSYS2_DLLS["$(tr '[:upper:]' '[:lower:]' <<<"$dll")"]="$dll"
done < <(cd "$MSYSTEM_PREFIX/bin" && ls -1 -- *.dll)

added=1
round=0
while ((added > 0)); do
    added=0
    ((++round))
    index_deployed "$DEPLOY_DIR"
    declare -A wanted=()
    while IFS= read -r file; do
        while IFS= read -r dep; do
            [[ -n "$dep" ]] || continue
            [[ -v "DEPLOYED[$dep]" ]] && continue
            is_system_dll "$dep" && continue
            wanted["$dep"]=1
        done < <(pe_imports "$DEPLOY_DIR/$file")
    done < <(pe_files "$DEPLOY_DIR")

    for dep in "${!wanted[@]}"; do
        if [[ -v "MSYS2_DLLS[$dep]" ]]; then
            cp "$MSYSTEM_PREFIX/bin/${MSYS2_DLLS[$dep]}" "$DEPLOY_DIR/"
            info "+ ${MSYS2_DLLS[$dep]}"
            ((++added))
        fi
        # Не найденные в MSYS2 и не системные — поймает check-deploy.sh.
    done
    unset wanted
done
info "проходов: $round"

step "Готово: $DEPLOY_DIR"
info "файлов: $(find "$DEPLOY_DIR" -type f | wc -l), размер: $(du -sh "$DEPLOY_DIR" | cut -f1)"
