
#!/usr/bin/env python3
from pathlib import Path
import json

root = Path(__file__).resolve().parents[1]
spec = json.loads((root / "buildspec.json").read_text())
obs = spec["dependencies"]["obs-studio"]
assert obs["hashes"].get("linux-x86_64"), "Linux OBS source hash must be pinned"
assert obs["baseUrl"].endswith("/archive/refs/tags")
assert obs["hashes"]["linux-x86_64"] == obs["hashes"]["macos"], (
    "Linux/macOS intentionally share the same platform-independent OBS source tarball hash"
)

linux_buildspec = root / "cmake/linux/buildspec.cmake"
assert linux_buildspec.exists(), "Linux must have an OBS source bootstrap"
text = linux_buildspec.read_text()
assert "set(dependencies_list obs-studio)" in text
assert "_check_dependencies()" in text

defaults = (root / "cmake/linux/defaults.cmake").read_text()
assert "EMULENS_BUILD_OBS_FROM_SOURCE" in defaults
assert "include(buildspec)" in defaults
assert "_emulens_pinned_libobs_dir" in defaults
assert "if(NOT libobs_DIR AND NOT DEFINED ENV{libobs_DIR})" in defaults
assert "NOT EMULENS_BUILD_OBS_FROM_SOURCE" in defaults, (
    "system libobs fallback must be bypassed for pinned source builds"
)

bootstrap = (root / "cmake/common/buildspec_common.cmake").read_text()
assert "elseif(OS_LINUX)" in bootstrap
assert 'set(_cmake_generator "Ninja")' in bootstrap
assert "libobs-opengl" in bootstrap, "CI SDK must build the OBS OpenGL runtime"
assert "--component Runtime" in bootstrap, "CI SDK must install OBS runtime libraries"
assert "-DCMAKE_INSTALL_PREFIX=${dependencies_dir}" in bootstrap, "libobs must resolve data from the pinned SDK prefix"

builder = (root / ".github/scripts/build-ubuntu").read_text()
assert "-DEMULENS_BUILD_OBS_FROM_SOURCE=ON" in builder

setup = (root / ".github/scripts/utils.zsh/setup_ubuntu").read_text()
for package in (
    "extra-cmake-modules",
    "libavcodec-dev",
    "libavformat-dev",
    "libjansson-dev",
    "libwayland-dev",
):
    assert package in setup, f"shared Ubuntu setup must provision {package}"
assert "ppa:obsproject/obs-studio" not in setup
package_lines = [line.strip().rstrip("\\").strip() for line in setup.splitlines()]
assert "obs-studio" not in package_lines, "CI must not install a runner-selected OBS package"

action = (root / ".github/actions/build-plugin/action.yaml").read_text()
assert "libavcodec-dev" not in action, "OBS source deps belong in shared setup_ubuntu"

obs33_builder = (root / ".github/scripts/build-obs33-compat.sh").read_text()
assert "--target obs-frontend-api libobs-opengl" in obs33_builder, (
    "OBS33 SDK install must build frontend API before Development install"
)

obs33_workflow = (root / ".github/workflows/obs33-compatibility.yml").read_text()
assert "libopengl-dev" in obs33_workflow, "OBS33 Ubuntu 26.04 gate needs GLVND OpenGL development library"

workflow = (root / ".github/workflows/ci.yml").read_text()
assert '.deps/lib/cmake/libobs' in workflow, "native tests must reuse the pinned CI SDK"
assert 'LD_LIBRARY_PATH' in workflow and '.deps/lib' in workflow, "GPU tests must load the pinned OBS runtime"
assert "runs-on: ubuntu-latest" not in workflow
assert workflow.count("runs-on: ubuntu-24.04") >= 3

obs33_workflow = (root / ".github/workflows/obs33-compatibility.yml").read_text()
assert "OBS 33.0.1" in obs33_workflow
assert "runs-on: ubuntu-26.04" in obs33_workflow
assert "build-obs33-compat.sh" in obs33_workflow
windows_helpers = (root / "cmake/windows/helpers.cmake").read_text()
assert 'DESTINATION "${target}/bin/64bit"' in windows_helpers
assert 'DESTINATION "${target}"' in windows_helpers
print("PASS: Linux CI pins runner and source-built OBS SDK")

root_cmake = (root / "CMakeLists.txt").read_text()
assert "find_package(libobs CONFIG REQUIRED)" in root_cmake
assert 'libobs_VERSION VERSION_LESS "32.1.0"' in root_cmake
assert "find_package(libobs 32.1.0 CONFIG REQUIRED)" not in root_cmake

tests_cmake = (root / "tests/CMakeLists.txt").read_text()
assert "find_package(libobs QUIET CONFIG)" in tests_cmake
assert 'libobs_VERSION VERSION_LESS "32.1.0"' in tests_cmake
assert "find_package(libobs 32.1 QUIET CONFIG)" not in tests_cmake
