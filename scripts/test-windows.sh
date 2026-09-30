#!/usr/bin/env bash
# Проверка развёрнутого приложения запуском под Wine (отдельный префикс build-win/wineprefix,
# невидимый дисплей xvfb). Запускает VolleyScout.exe --selftest: плагины, шрифты, тема,
# .xlsx и архив скаутов, воспроизведение видео, скриншоты окна в обеих темах.
#
#   scripts/test-windows.sh [папка с VolleyScout.exe]   (по умолчанию build-win/deploy)
set -euo pipefail
source "$(dirname "$0")/windows-env.sh"
require_tools wine winepath xvfb-run ffmpeg unzip python3

app_dir="${1:-$DEPLOY_DIR}"
[[ -f "$app_dir/VolleyScout.exe" ]] || fail "нет $app_dir/VolleyScout.exe"
use_project_wineprefix

test_dir="$BUILD_DIR/test"
video="$test_dir/test.mp4"
if [[ ! -f "$video" ]]; then
    step "Тестовое видео $video"
    mkdir -p "$test_dir"
    ffmpeg -loglevel error -y -f lavfi -i testsrc2=duration=6:size=1280x720:rate=25 \
        -f lavfi -i sine=duration=6 -c:v libx264 -preset veryfast -pix_fmt yuv420p -c:a aac -shortest "$video"
fi

run_dir="$test_dir/run-$(basename "$(dirname "$app_dir/VolleyScout.exe")")"
rm -rf "$run_dir"
mkdir -p "$run_dir"
log="$run_dir/wine.log"

step "Запуск под Wine: $app_dir/VolleyScout.exe --selftest"
set +e
# QT_FORCE_STDERR_LOGGING: у GUI-приложения на Windows сообщения Qt иначе уходят в OutputDebugString.
QT_DEBUG_PLUGINS=1 QT_FORCE_STDERR_LOGGING=1 WINEDEBUG="-all,err+module" \
    timeout 300 xvfb-run -a -s "-screen 0 1920x1080x24" \
    wine "$(winepath -w "$app_dir/VolleyScout.exe")" --selftest "$(winepath -w "$run_dir")" "$(winepath -w "$video")" \
    >"$log" 2>&1
code=$?
set -e
wineserver --wait || true
info "код выхода: $code, лог: $log"

errors=0
check() { if eval "$2"; then info "✓ $1"; else printf '\033[1;31m    ✗ %s\033[0m\n' "$1"; ((++errors)); fi; }

step "Результаты"
[[ -f "$run_dir/selftest.txt" ]] && sed 's/^/    /' "$run_dir/selftest.txt"
check "самопроверка приложения: все пункты OK" "[[ $code -eq 0 ]] && grep -q 'SELFTEST summary: OK' '$run_dir/selftest.txt'"
check "в логе нет ошибок загрузки DLL и плагинов" \
    "! grep -Ei 'err:module|Cannot load library|could not be loaded|Could not find the Qt platform plugin|Failed to load|not found' '$log'"
# Отладочный вывод загрузчика плагинов Qt в сборке MSYS2 не появляется, поэтому доказываем
# по факту: другого бэкенда мультимедиа в папке нет, а FFmpeg сам пишет, что открыл файл.
check "бэкенд мультимедиа — только FFmpeg" "[[ \$(ls '$app_dir/multimedia') == ffmpegmediaplugin.dll ]]"
check "видео открыто и декодировано FFmpeg" "grep -q '^Input #0' '$log' && grep -q 'SELFTEST video-playback: OK' '$run_dir/selftest.txt'"

zip="$(find "$run_dir" -maxdepth 1 -name '*.zip' | head -1)"
xlsx="$(find "$run_dir" -maxdepth 1 -name '*.xlsx' | head -1)"
check "архив скаутов распаковывается (unzip -t)" "[[ -n '$zip' ]] && unzip -tq '$zip' >/dev/null"
check "в архиве файлы партий *_s1.txt, *_s2.txt" "unzip -Z1 '$zip' | grep -q '_s1.txt' && unzip -Z1 '$zip' | grep -q '_s2.txt'"
check ".xlsx — корректная книга Excel (листы Матч, s1, s2)" "python3 - '$xlsx' <<'PY'
import sys, zipfile, re
z = zipfile.ZipFile(sys.argv[1]); assert z.testzip() is None
names = re.findall(r'<sheet [^>]*name=\"([^\"]+)\"', z.read('xl/workbook.xml').decode())
assert names == ['Матч', 's1', 's2'], names
PY"
check "скриншоты окна в обеих темах" "[[ \$(find '$run_dir/screens' -name '*.png' | wc -l) -eq 6 ]]"

echo
((errors == 0)) || fail "проверка под Wine не пройдена: ошибок $errors (лог $log)"
printf '\033[1;32mПроверка под Wine пройдена\033[0m (скриншоты: %s)\n' "$run_dir/screens"
