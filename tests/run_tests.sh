#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${EMULENS_TEST_BUILD_DIR:-$SCRIPT_DIR/build}"

cmake_args=(
  -S "$ROOT"
  -B "$BUILD_DIR"
  -DENABLE_TESTING=ON
)

obsdev_root="${OBSDEV_ROOT:-$HOME/.local/share/obsdev}"
if [[ -z "${libobs_DIR:-}" && -f "$obsdev_root/cmake/libobs/libobsConfig.cmake" ]]; then
  cmake_args+=("-Dlibobs_DIR=$obsdev_root/cmake/libobs")
fi

rm -rf "$BUILD_DIR"
cmake "${cmake_args[@]}"
cmake --build "$BUILD_DIR" --parallel "$(nproc)"

echo "Running CTest suite..."
ctest --test-dir "$BUILD_DIR/tests" --output-on-failure

echo "Running optional obspython integration test..."
if python3 -c "import obspython" 2>/dev/null; then
  python3 "$SCRIPT_DIR/test_integration.py"
else
  echo "Skipping obspython integration test (obspython not found)"
fi
