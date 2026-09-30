#!/usr/bin/env bash
# Кросс-сборка Windows-версии (Release) на Linux через quasi-msys2 (UCRT64, системный clang + lld).
# Результат: build-win/bin/VolleyScout.exe. Идемпотентен: повторный запуск пересобирает только изменённое.
set -euo pipefail
source "$(dirname "$0")/windows-env.sh"

require_tools clang ld.lld llvm-windres cmake ninja make wine magick

if [[ ! -f "$PROJECT_ROOT/src/apps/resources/windows/app.ico" ]]; then
    "$PROJECT_ROOT/scripts/generate-icons.sh"
fi

step "Окружение quasi-msys2 ($QUASI_MSYS2_DIR)"
enter_msys2_env
info "MSYSTEM=$MSYSTEM, префикс $MSYSTEM_PREFIX"

# QXlsx: переиспользуем исходники, уже скачанные Linux-сборкой, — без повторной загрузки.
qxlsx_args=()
for candidate in "$PROJECT_ROOT/build/_deps/qxlsx-src" "$BUILD_DIR/_deps/qxlsx-src"; do
    if [[ -f "$candidate/QXlsx/CMakeLists.txt" ]]; then
        qxlsx_args=("-DFETCHCONTENT_SOURCE_DIR_QXLSX=$candidate")
        info "QXlsx: $candidate"
        break
    fi
done

step "Конфигурация CMake → $BUILD_DIR"
# moc/rcc/uic — от системного Qt (QT_HOST_PATH): запускать .exe-версии под Wine слишком медленно.
cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DQT_HOST_PATH=/usr \
    -DVOLLEYSCOUT_BUILD_TESTS=OFF \
    "${qxlsx_args[@]}"

step "Сборка"
cmake --build "$BUILD_DIR" --parallel

exe="$BUILD_DIR/bin/VolleyScout.exe"
[[ -f "$exe" ]] || fail "не найден $exe"
info "готово: $exe ($(du -h "$exe" | cut -f1))"
