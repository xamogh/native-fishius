#!/bin/zsh
set -euo pipefail
PROJECT_DIR="${0:A:h}"
DEVICE_ID="E83F00A3-74DE-4021-92B0-3C3BD4E59DE1"
APP_PATH="$PROJECT_DIR/build/ios-simulator/aquarium.app"

if [[ ! -d "$APP_PATH" ]]; then
  print -u2 "The iPhone simulator build is missing. See LOCAL-SETUP.md."
  exit 1
fi
if ! xcrun simctl list devices booted | /usr/bin/grep -Fq "$DEVICE_ID"; then
  xcrun simctl boot "$DEVICE_ID"
fi
xcrun simctl bootstatus "$DEVICE_ID" -b
open -a Simulator --args -CurrentDeviceUDID "$DEVICE_ID"
xcrun simctl install "$DEVICE_ID" "$APP_PATH"
xcrun simctl launch "$DEVICE_ID" com.pixmot.aquarium
