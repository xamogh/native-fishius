#!/bin/zsh
set -euo pipefail
PROJECT_DIR="${0:A:h}"
cd "$PROJECT_DIR"
STUDIO_CMAKE="$(command -v cmake || true)"
if [[ -z "$STUDIO_CMAKE" && -f build/desktop/CMakeCache.txt ]]; then
  STUDIO_CMAKE="$(sed -n 's/^CMAKE_COMMAND:INTERNAL=//p' build/desktop/CMakeCache.txt)"
fi
if [[ ! -x "$STUDIO_CMAKE" ]]; then
  print "CMake is needed to build Aquarium Studio. Install CMake, then open this file again."
  exit 1
fi
"$STUDIO_CMAKE" --preset desktop -DAQ_BUILD_EDITOR=ON
"$STUDIO_CMAKE" --build --preset desktop --target aquarium_studio aquarium
exec "$PROJECT_DIR/build/desktop/aquarium_studio" --assets "$PROJECT_DIR/assets" "$@"
