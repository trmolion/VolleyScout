#!/usr/bin/env bash
# app.ico для Windows из resources/icons/app-logo.svg (currentColor → accent «Полдня» #0B7F80).
# Готовый .ico лежит в репозитории; скрипт нужен только при смене логотипа.
set -euo pipefail
source "$(dirname "$0")/windows-env.sh"
require_tools magick

svg="$PROJECT_ROOT/src/apps/resources/icons/app-logo.svg"
ico="$PROJECT_ROOT/src/apps/resources/windows/app.ico"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

step "Генерация $ico"
sed 's/currentColor/#0B7F80/g' "$svg" >"$tmp/app-logo.svg"
magick -background none -density 1024 "$tmp/app-logo.svg" \
    -define icon:auto-resize=256,128,64,48,32,24,16 "$ico"
magick identify "$ico" | awk '{print "    " $3}' | tr '\n' ' '; echo
