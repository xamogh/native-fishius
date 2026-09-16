#!/bin/zsh
set -euo pipefail
PROJECT_DIR="${0:A:h}"
BUILD_DIR="$PROJECT_DIR/build/ios-simulator-xcode"
CACHE_PATH="$BUILD_DIR/CMakeCache.txt"

if [[ ! -f "$CACHE_PATH" ]]; then
  print -u2 "The Xcode simulator project is missing. See LOCAL-SETUP.md."
  exit 1
fi

CMAKE_BIN="$(sed -n 's/^CMAKE_COMMAND:INTERNAL=//p' "$CACHE_PATH")"
if [[ ! -x "$CMAKE_BIN" ]]; then
  print -u2 "The configured CMake tool is missing. See LOCAL-SETUP.md."
  exit 1
fi

"$CMAKE_BIN" -S "$PROJECT_DIR" -B "$BUILD_DIR"
open -a Xcode "$BUILD_DIR/FishiusAquarium.xcodeproj"
