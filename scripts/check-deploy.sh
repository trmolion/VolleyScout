#!/usr/bin/env bash
# Проверка build-win/deploy/ (или папки из аргумента) перед упаковкой. Не доверяет windeployqt:
#   1. рекурсивно по всем .exe/.dll (включая плагины) читает таблицы импортов (llvm-readobj)
#      и требует, чтобы каждая DLL лежала в папке или была системной (windows-system-dlls.txt);
#   2. проверяет обязательные плагины Qt, FFmpeg и рантайм компилятора;
#   3. проверяет, что нет мусора (отладочные DLL, .a, .pdb, …);
#   4. выводит размер папки и топ-10 файлов.
# Код выхода ≠ 0 при любой ошибке.
set -euo pipefail
source "$(dirname "$0")/windows-env.sh"
source "$(dirname "$0")/pe-imports.sh"
require_tools llvm-readobj

dir="${1:-$DEPLOY_DIR}"
[[ -d "$dir" ]] || fail "нет папки $dir"
errors=0
error() { printf '\033[1;31m    ✗ %s\033[0m\n' "$*"; ((++errors)); }
ok() { printf '    ✓ %s\n' "$*"; }

load_system_dlls
index_deployed "$dir"

step "1. Зависимости всех .exe и .dll (рекурсивно)"
files=0
while IFS= read -r file; do
    ((++files))
    while IFS= read -r dep; do
        [[ -n "$dep" ]] || continue
        [[ -v "DEPLOYED[$dep]" ]] && continue
        is_system_dll "$dep" && continue
        error "$dep — не найдена, нужна для $file"
    done < <(pe_imports "$dir/$file")
done < <(pe_files "$dir")
((errors == 0)) && ok "проверено файлов: $files, все зависимости на месте"

step "2. Обязательные компоненты"
require_file() { # <путь> <зачем>
    if [[ -f "$dir/$1" ]]; then ok "$1 — $2"; else error "нет $1 — $2"; fi
}
require_file VolleyScout.exe "приложение"
require_file platforms/qwindows.dll "платформа Qt"
require_file imageformats/qsvg.dll "SVG-изображения"
require_file iconengines/qsvgicon.dll "SVG-иконки"
require_file multimedia/ffmpegmediaplugin.dll "бэкенд QMediaPlayer (FFmpeg)"
require_file styles/qmodernwindowsstyle.dll "стиль Windows (база Fusion не требует, но windeployqt кладёт)"
for lib in Qt6Core Qt6Gui Qt6Widgets Qt6Svg Qt6Multimedia Qt6MultimediaWidgets; do
    require_file "$lib.dll" "модуль Qt"
done
for pattern in 'avcodec-*.dll' 'avformat-*.dll' 'avutil-*.dll' 'swscale-*.dll' 'swresample-*.dll'; do
    found="$(cd "$dir" && compgen -G "$pattern" | head -1 || true)"
    if [[ -n "$found" ]]; then ok "$found — FFmpeg"; else error "нет $pattern — FFmpeg"; fi
done
for lib in libstdc++-6.dll libgcc_s_seh-1.dll libwinpthread-1.dll; do
    require_file "$lib" "рантайм компилятора (UCRT64/GCC)"
done
# Шрифты и SVG-иконки вшиты в exe через qrc — проверяем, что они там есть.
if grep -qa "IBMPlexSans\|JetBrainsMono" "$dir/VolleyScout.exe" || strings -el "$dir/VolleyScout.exe" | grep -q "IBMPlexSans"; then
    ok "шрифты и иконки — внутри VolleyScout.exe (qrc)"
else
    error "в VolleyScout.exe не найдены ресурсы шрифтов (qrc)"
fi

step "3. Лишние файлы"
junk=0
while IFS= read -r file; do
    error "лишний файл: $file"
    ((++junk))
done < <(cd "$dir" && find . -type f \( -iname '*.a' -o -iname '*.lib' -o -iname '*.pdb' -o -iname '*.debug' \
    -o -iname '*.exp' -o -iname '*.ilk' -o -iname '*.o' -o -iname '*.obj' -o -iname '*.log' \) -printf '%P\n')
# Отладочные сборки Qt: Qt6Cored.dll и т.п. (у libdav1d.dll «d» — часть имени, поэтому
# отладочной считаем DLL, у которой рядом есть такая же без суффикса «d»).
while IFS= read -r file; do
    base="${file%d.dll}.dll"
    if [[ -f "$dir/$base" ]]; then error "отладочная DLL: $file"; ((++junk)); fi
done < <(cd "$dir" && find . -type f -iname '*d.dll' -printf '%P\n')
((junk == 0)) && ok "мусора нет"

step "4. Размер"
info "итого: $(du -sh "$dir" | cut -f1), файлов: $(find "$dir" -type f | wc -l)"
info "топ-10:"
(cd "$dir" && find . -type f -printf '%s\t%P\n' | sort -rn | head -10 |
    awk -F'\t' '{ printf "      %7.1f МБ  %s\n", $1 / 1048576, $2 }')

echo
if ((errors > 0)); then
    fail "проверка деплоя не пройдена: ошибок $errors"
fi
printf '\033[1;32mПроверка деплоя пройдена\033[0m\n'
