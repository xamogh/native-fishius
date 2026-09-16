#!/bin/zsh
set -euo pipefail
PROJECT_DIR="${0:A:h}"
PROJECT_PATH="$PROJECT_DIR/build/ios-device/FishiusAquarium.xcodeproj"
CACHE_PATH="$PROJECT_DIR/build/ios-device/CMakeCache.txt"

if [[ ! -d "$PROJECT_PATH" || ! -f "$CACHE_PATH" ]]; then
  print -u2 "The iPhone project is missing. See docs/mobile.md and LOCAL-SETUP.md."
  exit 1
fi

CMAKE_BIN="$(sed -n 's/^CMAKE_COMMAND:INTERNAL=//p' "$CACHE_PATH")"
if [[ ! -x "$CMAKE_BIN" ]]; then
  print -u2 "The configured CMake tool is missing. See LOCAL-SETUP.md."
  exit 1
fi

"$CMAKE_BIN" -S "$PROJECT_DIR" -B "$PROJECT_DIR/build/ios-device"
open -a Xcode "$PROJECT_PATH"
