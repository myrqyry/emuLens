#!/usr/bin/env bash
# Build, stage, then swap a verified plugin tree. Never edit the running OBS profile.
set -Eeuo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
INSTALL_PREFIX="${EMULENS_INSTALL_PREFIX:-$HOME/.config/obs-studio/plugins/emuLensFX}"
BUILD_DIR="${EMULENS_BUILD_DIR:-$ROOT/build}"
STAGING_ROOT="${TMPDIR:-/tmp}"
STAGING_PREFIX=""
PREPARED=""
BACKUP=""
SWAPPED=0

cleanup() {
  local status=$?
  if [[ "$SWAPPED" -eq 0 && -n "$BACKUP" && -d "$BACKUP" && ! -e "$INSTALL_PREFIX" ]]; then
    mv -- "$BACKUP" "$INSTALL_PREFIX" || {
      echo "Recovery needed: previous plugin remains in $BACKUP" >&2
      status=1
    }
  fi
  if [[ -n "$STAGING_PREFIX" && -d "$STAGING_PREFIX" ]]; then rm -rf -- "$STAGING_PREFIX"; fi
  if [[ -n "$PREPARED" && -d "$PREPARED" ]]; then rm -rf -- "$PREPARED"; fi
  exit "$status"
}
trap cleanup EXIT

case "$(uname -m)" in
  x86_64) OBS_ARCH_DIR="x86_64-linux-gnu" ;;
  aarch64) OBS_ARCH_DIR="aarch64-linux-gnu" ;;
  *) echo "Unsupported Linux architecture: $(uname -m)" >&2; exit 1 ;;
esac

OBS_CMAKE_DIR="${OBS_CMAKE_DIR:-${libobs_DIR:-/usr/lib/$OBS_ARCH_DIR/cmake/libobs}}"

# Stage first: a failed build or incomplete plugin never touches the existing install.
mkdir -p "$STAGING_ROOT"
STAGING_PREFIX="$(mktemp -d "$STAGING_ROOT/emulensfx-install.XXXXXX")"
cmake -S "$ROOT" -B "$BUILD_DIR" \
  -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" -Dlibobs_DIR="$OBS_CMAKE_DIR"
cmake --build "$BUILD_DIR" --parallel "$(nproc)"
cmake --install "$BUILD_DIR" --prefix "$STAGING_PREFIX"

# GNUInstallDirs selects lib/, lib64/ or a distro multiarch lib/<triplet>/.
# Resolve the staged module under obs-plugins rather than assuming one layout.
module=""
if [[ -d "$STAGING_PREFIX/lib" ]]; then
  module="$(find "$STAGING_PREFIX/lib" -type f -path '*/obs-plugins/emuLensFX.so' -print -quit)"
fi
if [[ -z "$module" && -d "$STAGING_PREFIX/lib64" ]]; then
  module="$(find "$STAGING_PREFIX/lib64" -type f -path '*/obs-plugins/emuLensFX.so' -print -quit)"
fi
assets="$STAGING_PREFIX/share/obs/obs-plugins/emuLensFX"
if [[ -z "$module" || ! -s "$module" || ! -s "$assets/emulens.effect" || ! -s "$assets/locale/en-US.ini" ]]; then
  echo "Incomplete staged plugin: refusing to replace working installation" >&2
  exit 1
fi

# A non-empty file is not necessarily a shared library. Regression tests
# previously wrote a 10-byte text fixture under the name emuLensFX.so.
# Validate the native ELF, architecture, and OBS entry points before the swap.
if ! command -v readelf >/dev/null || ! command -v nm >/dev/null; then
  echo "Cannot verify plugin binary: readelf and nm are required" >&2
  exit 1
fi
if ! elf_header="$(readelf -h "$module" 2>/dev/null)" || [[ "$elf_header" != *'DYN (Shared object file)'* ]]; then
  echo "Staged emuLensFX.so is not an ELF shared library; keeping existing plugin" >&2
  exit 1
fi
case "$(uname -m)" in
  x86_64) expected_machine='Advanced Micro Devices X86-64' ;;
  aarch64) expected_machine='AArch64' ;;
esac
if [[ "$elf_header" != *"$expected_machine"* ]]; then
  echo "Staged emuLensFX.so has the wrong CPU architecture; keeping existing plugin" >&2
  exit 1
fi
# Imported (U) symbols cannot satisfy OBS's dlopen module interface.
# Match exact names in the dynamic symbol definitions, not substrings.
if ! symbols="$(nm -D --defined-only "$module" 2>/dev/null)" ||
   ! awk '
       $3 == "obs_module_load" { has_load=1 }
       $3 == "obs_module_ver"  { has_ver=1 }
       END { exit !(has_load && has_ver) }
   ' <<< "$symbols"; then
  echo "Staged binary is missing required exported OBS module entry points" >&2
  exit 1
fi

# A sibling prepare directory ensures final rename stays on the same filesystem.
parent="$(dirname "$INSTALL_PREFIX")"
mkdir -p "$parent"
PREPARED="$(mktemp -d "$parent/.emuLensFX.prepare.XXXXXX")"
if [[ -d "$INSTALL_PREFIX" ]]; then
  cp -a "$INSTALL_PREFIX/." "$PREPARED/"
fi
rm -rf -- "$PREPARED/bin" "$PREPARED/data" "$PREPARED/lib" "$PREPARED/share"
rm -f -- "$PREPARED/emuLensFX.so"
mkdir -p "$PREPARED/bin/64bit" "$PREPARED/data"
# OBS 33+ loads third-party modules from the plugin root. Keep the legacy
# bin/64bit copy so the same install remains usable with OBS 32 and older.
cp -p "$module" "$PREPARED/emuLensFX.so"
cp -p "$module" "$PREPARED/bin/64bit/emuLensFX.so"
cp -a "$assets/." "$PREPARED/data/"
test -s "$PREPARED/emuLensFX.so"
cmp -s "$module" "$PREPARED/emuLensFX.so"
test -s "$PREPARED/bin/64bit/emuLensFX.so"
cmp -s "$module" "$PREPARED/bin/64bit/emuLensFX.so"
test -s "$PREPARED/data/emulens.effect"
test -s "$PREPARED/data/locale/en-US.ini"

# Preserve the old installation until the new one is fully prepared.
if [[ -e "$INSTALL_PREFIX" ]]; then
  BACKUP="$(mktemp -d "$parent/.emuLensFX.backup.XXXXXX")"
  rmdir "$BACKUP"
  mv -- "$INSTALL_PREFIX" "$BACKUP"
fi
mv -- "$PREPARED" "$INSTALL_PREFIX"
PREPARED=""
SWAPPED=1
if [[ -n "$BACKUP" ]]; then rm -rf -- "$BACKUP"; fi
echo "Done. Restart OBS to load the updated EmuLens filter."
echo "Installed to: $INSTALL_PREFIX"
