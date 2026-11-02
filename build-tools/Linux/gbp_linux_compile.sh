#!/usr/bin/env bash
# =============================================================================
#  gbp_linux_compile.sh — Compile gbp in "Release" without Qt Creator
# =============================================================================
#
#  Usage : ./gbp_linux_compile.sh             "Embedded manuals version"
#          ./gbp_linux_compile.sh nomanual    "Online manuals version"
#
#  The "Embedded manuals version" has the User Manual, Quick Tutorial and
#  Change Log PDFs built into the executable. The "Online manuals version" only shows their
#  address in the GitHub repository (GBP never accesses the Internet) (CMake
#  option GBP_EMBEDDED_MANUALS=OFF). Both have the Welcome PDF built in.
#  Each version has its own build directory (see BUILD_DIR below).
#
#  Configures and compiles gbp with CMake and Ninja, using the Qt installed
#  with the Qt online installer. It is always a full rebuild : the build
#  directory is deleted first.
#
#  The result goes in the same build directory as Qt Creator's "Release"
#  configuration (followed by "-nomanual" for the "Online manuals version"),
#  the one used by BUILDING.md (Step 6) to make the AppImage :
#    - graphical-budget-planner      (the executable)
#    - gbp_en.qm, gbp_fr.qm          (the translation files)
#  The translation sources gbp_en.ts and gbp_fr.ts in "src" are also updated
#  (lupdate), as with Qt Creator.
#
#  The 3 directories below can be changed here, or overridden for one run
#  from the command line, e.g. :
#    QT_DIR=/opt/Qt/6.11.3/gcc_64 ./gbp_linux_compile.sh
# =============================================================================

# Stop on errors (-e), on unset variables (-u), and on failures inside
# pipelines (-o pipefail)
set -euo pipefail

# -----------------------------------------------------------------------------
#  CONFIGURATION
# -----------------------------------------------------------------------------
QT_VERSION="${QT_VERSION:-6.11.3}"
# Same version with "_" instead of "." (e.g. 6_11_3), as in the Qt Creator build folder name
QT_VERSION_U="${QT_VERSION//./_}"

QT_DIR="${QT_DIR:-$HOME/Qt/$QT_VERSION/gcc_64}"
SRC_DIR="${SRC_DIR:-$HOME/data/dev/Qt/gbp/src}"

die() { echo "ERROR : $*" >&2; exit 1; }

DEFAULT_BUILD_DIR="$HOME/data/dev/Qt/build/gbp/Desktop_Qt_${QT_VERSION_U}-Release"
if [[ $# -eq 0 ]]; then
    EMBEDDED_MANUALS=ON
    BUILD_DIR="${BUILD_DIR:-$DEFAULT_BUILD_DIR}"
elif [[ $# -eq 1 && "$1" == "nomanual" ]]; then
    EMBEDDED_MANUALS=OFF
    BUILD_DIR="${BUILD_DIR:-$DEFAULT_BUILD_DIR-nomanual}"
else
    die "unknown argument \"$*\". Usage : $0  [nomanual]"
fi

# -----------------------------------------------------------------------------
#  CHECKS
# -----------------------------------------------------------------------------
[[ -x "$QT_DIR/bin/qmake" ]] || die "Qt $QT_VERSION not found in $QT_DIR (set QT_DIR)"
[[ -f "$SRC_DIR/CMakeLists.txt" ]] || die "gbp sources not found in $SRC_DIR (set SRC_DIR)"

# CMake and Ninja : the ones installed with Qt if present, otherwise the system ones
# (Ubuntu packages "cmake" and "ninja-build")
QT_ROOT="$(dirname "$(dirname "$QT_DIR")")"   # e.g. ~/Qt
for d in "$QT_ROOT/Tools/CMake/bin" "$QT_ROOT/Tools/Ninja"; do
    if [[ -d "$d" ]]; then PATH="$d:$PATH"; fi
done
command -v cmake >/dev/null || die "cmake not found (sudo apt install cmake)"
command -v ninja >/dev/null || die "ninja not found (sudo apt install ninja-build)"
command -v g++   >/dev/null || die "g++ not found (sudo apt install build-essential)"

# -----------------------------------------------------------------------------
#  BUILD
# -----------------------------------------------------------------------------
# Always a full rebuild, from an empty build directory (also avoids conflicts with a build
# directory configured differently by Qt Creator)
rm -rf "$BUILD_DIR"

echo "Qt        : $QT_DIR"
echo "Sources   : $SRC_DIR"
echo "Build dir : $BUILD_DIR"
echo "Embedded manuals : $EMBEDDED_MANUALS"

# Configure, then build with all the processor cores
cmake -S "$SRC_DIR" -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$QT_DIR" -DGBP_EMBEDDED_MANUALS="$EMBEDDED_MANUALS" || die "CMake configuration failed"
cmake --build "$BUILD_DIR" --parallel || die "build failed"

echo
echo "Built $BUILD_DIR/graphical-budget-planner"
