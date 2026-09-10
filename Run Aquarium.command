#!/bin/zsh
set -euo pipefail
PROJECT_DIR="${0:A:h}"
cd "$PROJECT_DIR"
mkdir -p local-data
exec "$PROJECT_DIR/build/desktop/aquarium" \
  --assets "$PROJECT_DIR/assets" \
  --save "$PROJECT_DIR/local-data/save.json" "$@"
