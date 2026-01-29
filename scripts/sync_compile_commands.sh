#!/usr/bin/env bash
set -euo pipefail

# Copies <build_dir>/compile_commands.json to ./compile_commands.json
# Usage:
#   ./scripts/sync_compile_commands.sh build/debug
#   ./scripts/sync_compile_commands.sh build/asan

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <build_dir>"
  exit 2
fi

build_dir="$1"
src="${build_dir%/}"/compile_commands.json
dst="compile_commands.json"

if [[ ! -f "$src" ]]; then
  echo "error: not found: $src"
  echo "-DCMAKE_EXPORT_COMPILE_COMMANDS is probably not =ON"
  exit 1
fi

cp -f "$src" "$dst"
echo "copied $src -> $dst"
