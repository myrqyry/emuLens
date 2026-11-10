#!/usr/bin/env python3
"""Installer safety contracts: run with an isolated HOME and staged binary."""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
INSTALLER = ROOT / "install_local.sh"


class InstallerSafety(unittest.TestCase):
    def run_install(self, *, incomplete=False, fail_swap=False, multiarch=False,
                    invalid_binary=False, undefined_exports=False):
        with tempfile.TemporaryDirectory(prefix="emulens-install-test-") as tmp:
            root = Path(tmp)
            bindir = root / "bin"
            bindir.mkdir()
            home = root / "isolated-home"
            (home / ".config").mkdir(parents=True)
            staging = root / "staging"
            staging.mkdir()
            marker = root / "incomplete"
            if incomplete:
                marker.touch()

            # Use a real ELF shared library with OBS module entry points to
            # distinguish a valid staged binary from a short text fixture.
            module = root / "valid-module.so"
            subprocess.run(
                ["cc", "-shared", "-fPIC", "-x", "c", "-o", str(module), "-"],
                input=("__attribute__((visibility(\"default\"))) "
                       "int obs_module_load(void) {return 1;}\n"
                       "__attribute__((visibility(\"default\"))) "
                       "int obs_module_ver(void) {return 1;}\n"),
                text=True, capture_output=True, check=True,
            )
            # A valid ELF may import the OBS symbols without defining them.
            # That cannot satisfy OBS's module loader contract.
            if undefined_exports:
                subprocess.run(
                    ["cc", "-shared", "-fPIC", "-x", "c", "-o", str(module), "-"],
                    input=("extern int obs_module_load(void);\n"
                           "extern int obs_module_ver(void);\n"
                           "int reference_only(void) { return obs_module_load() + obs_module_ver(); }\n"),
                    text=True, capture_output=True, check=True,
                )
            cmake = bindir / "cmake"
            cmake.write_text("""#!/bin/sh
set -eu
case "$1" in
  --install)
    while [ "$#" -gt 0 ]; do
      if [ "$1" = --prefix ]; then shift; dst="$1"; fi
      shift
    done
    libdir=lib
    if [ "$MULTIARCH_LAYOUT" = 1 ]; then libdir=lib/x86_64-linux-gnu; fi
    mkdir -p "$dst/$libdir/obs-plugins" "$dst/share/obs/obs-plugins/emuLensFX/presets"
    if [ ! -e "$INCOMPLETE_MARKER" ]; then
      if [ "$INVALID_BINARY" = 1 ]; then
        printf 'new plugin' > "$dst/$libdir/obs-plugins/emuLensFX.so"
      else
        cp "$VALID_LIBRARY" "$dst/$libdir/obs-plugins/emuLensFX.so"
      fi
    fi
    printf 'new preset' > "$dst/share/obs/obs-plugins/emuLensFX/presets/test.json"
    printf 'effect' > "$dst/share/obs/obs-plugins/emuLensFX/emulens.effect"
    mkdir -p "$dst/share/obs/obs-plugins/emuLensFX/locale"
    printf 'locale' > "$dst/share/obs/obs-plugins/emuLensFX/locale/en-US.ini"
    ;;
  -S|--build) exit 0 ;;
  *) echo "Unexpected CMake invocation: $*" >&2; exit 90 ;;
esac
""")
            cmake.chmod(0o755)
            if fail_swap:
                mv = bindir / "mv"
                mv.write_text("""#!/bin/sh
case "$2" in
  */.emuLensFX.prepare.*) exit 76 ;;
esac
case "$3" in
  */plugins/emuLensFX)
    case "$2" in */.emuLensFX.prepare.*) exit 76 ;; esac
    ;;
esac
exec /usr/bin/mv "$@"
""")
                mv.chmod(0o755)
            prefix = root / "obs-config" / "plugins" / "emuLensFX"
            (prefix / "bin" / "64bit").mkdir(parents=True)
            (prefix / "data").mkdir()
            (prefix / "bin" / "64bit" / "emuLensFX.so").write_text("previous working plugin")
            (prefix / "custom-user-file.txt").write_text("keep my file")
            env = os.environ.copy()
            env.update({
                # This is essential: even an earlier installer that ignored
                # EMULENS_INSTALL_PREFIX cannot access the real OBS profile.
                "HOME": str(home),
                "XDG_CONFIG_HOME": str(home / ".config"),
                "TMPDIR": str(staging),
                "PATH": f"{bindir}:{env['PATH']}",
                "EMULENS_INSTALL_PREFIX": str(prefix),
                "EMULENS_BUILD_DIR": str(root / "build"),
                "INCOMPLETE_MARKER": str(marker),
                "MULTIARCH_LAYOUT": "1" if multiarch else "0",
                "INVALID_BINARY": "1" if invalid_binary else "0",
                "VALID_LIBRARY": str(module),
            })
            self.assertNotEqual(env["HOME"], str(Path.home()))
            result = subprocess.run(["bash", str(INSTALLER)], env=env, cwd=root,
                                    capture_output=True, text=True, timeout=30)
            installed = prefix / "bin" / "64bit" / "emuLensFX.so"
            modern = prefix / "emuLensFX.so"
            self.assertTrue(installed.exists(), result.stderr)
            self.assertEqual((prefix / "custom-user-file.txt").read_text(), "keep my file")
            failed = incomplete or fail_swap or invalid_binary or undefined_exports
            self.assertEqual(installed.read_bytes(),
                             b"previous working plugin" if failed else module.read_bytes())
            if failed:
                self.assertFalse(modern.exists(), "failed install must not create an OBS 33 root copy")
            else:
                self.assertEqual(modern.read_bytes(), module.read_bytes())
            self.assertEqual(result.returncode == 0, not failed,
                             result.stderr + result.stdout)
            if not failed:
                self.assertEqual((prefix / "data" / "presets" / "test.json").read_text(), "new preset")
            self.assertEqual(list(prefix.parent.glob(".emuLensFX.prepare.*")), [])
            self.assertEqual(list(prefix.parent.glob(".emuLensFX.backup.*")), [])

    def test_incomplete_staged_install_cannot_destroy_existing_plugin(self):
        self.run_install(incomplete=True)

    def test_successful_install_preserves_unrelated_files(self):
        self.run_install()

    def test_swap_failure_restores_original_plugin(self):
        self.run_install(fail_swap=True)

    def test_multiarch_debian_libdir_installs_without_losing_original(self):
        self.run_install(multiarch=True)

    def test_text_file_disguised_as_plugin_must_never_be_installed(self):
        self.run_install(invalid_binary=True)

    def test_imported_but_undefined_obs_symbols_cannot_replace_working_plugin(self):
        self.run_install(undefined_exports=True)


if __name__ == "__main__":
    unittest.main()
