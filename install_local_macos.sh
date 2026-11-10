#!/usr/bin/env bash
set -euo pipefail

BUILD_TYPE="${BUILD_TYPE:-Release}"
INSTALL_PREFIX="$HOME/Library/Application Support/obs-studio/plugins/emuLensFX"

echo "================================================"
echo "  Building EmuLensFX for macOS"
echo "================================================"
echo "Install prefix: $INSTALL_PREFIX"

mkdir -p build

ARCH="$(uname -m)"
if [[ "$ARCH" == "arm64" ]]; then
  CMAKE_ARCH="arm64"
else
  CMAKE_ARCH="x86_64"
fi

OBS_DIR_ARGS=()
if command -v brew >/dev/null 2>&1 && brew list obs >/dev/null 2>&1; then
  OBS_PREFIX="$(brew --prefix obs)"
  OBS_DIR_ARGS+=("-DOBS_DIR=$OBS_PREFIX")
  echo "Using Homebrew OBS at: $OBS_PREFIX"
fi

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DCMAKE_OSX_ARCHITECTURES="$CMAKE_ARCH" \
  -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
  -DOBS_PLUGIN_DESTINATION="bin" \
  -DOBS_DATA_DESTINATION="data" \
  "${OBS_DIR_ARGS[@]}"

cmake --build build --parallel "$(sysctl -n hw.ncpu)"
cmake --install build --prefix "$INSTALL_PREFIX"

echo ""
echo "✓ Installation complete. Restart OBS Studio."
echo "Plugin installed to: $INSTALL_PREFIX"
