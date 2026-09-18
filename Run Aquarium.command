#!/bin/zsh
set -euo pipefail
PROJECT_DIR="${0:A:h}"
cd "$PROJECT_DIR"
"$PROJECT_DIR/tools/build_game.sh" desktop
mkdir -p local-data
exec "$PROJECT_DIR/build/desktop/aquarium" \
  --assets "$PROJECT_DIR/assets" \
  --save "$PROJECT_DIR/local-data/save.json" "$@"
