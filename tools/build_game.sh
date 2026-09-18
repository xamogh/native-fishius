#!/bin/sh
set -eu

PROJECT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BUILD_NAME=${1:-desktop}
case "$BUILD_NAME" in
  desktop|ios-simulator) ;;
  *) printf 'Unknown game build: %s\n' "$BUILD_NAME" >&2; exit 1 ;;
esac
BUILD_DIR="$PROJECT_DIR/build/$BUILD_NAME"
GAME_CMAKE=$(command -v cmake || true)
if [ -z "$GAME_CMAKE" ] && [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
  GAME_CMAKE=$(sed -n 's/^CMAKE_COMMAND:INTERNAL=//p' "$BUILD_DIR/CMakeCache.txt")
fi
if [ ! -x "$GAME_CMAKE" ]; then
  printf 'CMake is needed to build the game. See LOCAL-SETUP.md.\n' >&2
  exit 1
fi
if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
  if [ "$BUILD_NAME" = desktop ]; then
    "$GAME_CMAKE" -S "$PROJECT_DIR" -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Release
  else
    printf 'Configure the iOS simulator build first. See LOCAL-SETUP.md.\n' >&2
    exit 1
  fi
fi
exec "$GAME_CMAKE" --build "$BUILD_DIR" --target aquarium --parallel 4
