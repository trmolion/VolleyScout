# Общие настройки Windows-сборки. Подключается через `source` из остальных скриптов.
#
# QUASI_MSYS2_DIR — где установлен quasi-msys2 (не QUASI_MSYS2_ROOT: это имя занято самим quasi-msys2) (окружение UCRT64).
# Выносится в переменную, чтобы можно было держать его, например, в third_party/.

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
QUASI_MSYS2_DIR="${QUASI_MSYS2_DIR:-$HOME/.local/share/quasi-msys2}"

BUILD_DIR="${BUILD_DIR:-$PROJECT_ROOT/build-win}"
DEPLOY_DIR="$BUILD_DIR/deploy"
DIST_DIR="$PROJECT_ROOT/dist"
WINEPREFIX_DIR="$BUILD_DIR/wineprefix"

step() { printf '\n\033[1;36m==> %s\033[0m\n' "$*"; }
info() { printf '    %s\n' "$*"; }
fail() { printf '\033[1;31mОШИБКА: %s\033[0m\n' "$*" >&2; exit 1; }

require_tools() {
    local missing=()
    for tool in "$@"; do command -v "$tool" >/dev/null 2>&1 || missing+=("$tool"); done
    [[ ${#missing[@]} -eq 0 ]] || fail "не найдены утилиты: ${missing[*]} (см. README, раздел «Сборка под Windows»)"
}

# Отдельный чистый Wine-префикс проекта — пользовательский ~/.wine не трогаем.
use_project_wineprefix() {
    export WINEPREFIX="$WINEPREFIX_DIR"
    export WINEDEBUG="${WINEDEBUG:--all}"
    export WINEDLLOVERRIDES="${WINEDLLOVERRIDES:-mscoree,mshtml=}" # без запроса на установку Mono/Gecko
    if [[ ! -f "$WINEPREFIX/system.reg" ]]; then
        info "создаю Wine-префикс $WINEPREFIX"
        mkdir -p "$WINEPREFIX"
        wineboot --init >/dev/null 2>&1 || fail "wineboot --init"
        wineserver --wait
    fi
}

# Окружение quasi-msys2 без binfmt_misc (он требует sudo, а Wine мы вызываем явно):
# обёртки для MSYS2-утилит + переменные (MSYSTEM_PREFIX, флаги кросс-компиляции, обёртка cmake).
enter_msys2_env() {
    [[ -f "$QUASI_MSYS2_DIR/env/vars.src" ]] || fail "quasi-msys2 не найден в $QUASI_MSYS2_DIR (переменная QUASI_MSYS2_DIR)"
    # quasi-msys2 при подготовке запускает MSYS2-утилиты под Wine — только в проектном префиксе.
    use_project_wineprefix
    make -f "$QUASI_MSYS2_DIR/env/fakebin.mk" QUIET=1 >/dev/null || fail "quasi-msys2: fakebin.mk"
    # vars.src рассчитан на интерактивную оболочку и местами обращается к неустановленным переменным.
    set +u
    unset QUASI_MSYS2_ROOT # признак «окружение уже активно» для vars.src
    local QUASI_MSYS2_QUIET=1
    # shellcheck disable=SC1091
    mkdir -p "$BUILD_DIR"
    source "$QUASI_MSYS2_DIR/env/vars.src" >"$BUILD_DIR/msys2-env.log" 2>&1 || fail "quasi-msys2: vars.src (см. $BUILD_DIR/msys2-env.log)"
    set -u
    [[ -n "${MSYSTEM_PREFIX:-}" && -d "$MSYSTEM_PREFIX" ]] || fail "quasi-msys2: не задан MSYSTEM_PREFIX"
}

