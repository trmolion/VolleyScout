# Разбор таблицы импортов PE (обычной и отложенной) через нативный llvm-readobj.
# Подключается через `source`.

# pe_imports <file> — имена импортируемых DLL в нижнем регистре, по одному на строку.
pe_imports() {
    llvm-readobj --coff-imports "$1" 2>/dev/null |
        awk '/^\s*Name: .*\.(dll|DLL|drv|DRV)\s*$/ { print tolower($2) }' | sort -u
}

# Загрузка белого списка системных DLL в массив SYSTEM_DLL_PATTERNS.
load_system_dlls() {
    SYSTEM_DLL_PATTERNS=()
    while IFS= read -r line; do
        line="${line%%#*}"
        line="$(echo "$line" | tr -d '[:space:]' | tr '[:upper:]' '[:lower:]')"
        [[ -n "$line" ]] && SYSTEM_DLL_PATTERNS+=("$line")
    done <"$(dirname "${BASH_SOURCE[0]}")/windows-system-dlls.txt"
}

is_system_dll() {
    local name="$1" pattern
    for pattern in "${SYSTEM_DLL_PATTERNS[@]}"; do
        # shellcheck disable=SC2053
        [[ "$name" == $pattern ]] && return 0
    done
    return 1
}

# Все .exe и .dll в папке рекурсивно (включая плагины), пути относительно неё.
pe_files() {
    (cd "$1" && find . -type f \( -iname '*.exe' -o -iname '*.dll' \) -printf '%P\n' | sort)
}

# Карта «имя в нижнем регистре → путь» для DLL в папке (рекурсивно).
declare -gA DEPLOYED
index_deployed() {
    DEPLOYED=()
    local file
    while IFS= read -r file; do
        DEPLOYED["$(basename "$file" | tr '[:upper:]' '[:lower:]')"]="$file"
    done < <(pe_files "$1")
}
