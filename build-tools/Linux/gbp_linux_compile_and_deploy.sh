#!/usr/bin/env bash
# =============================================================================
#  gbp_linux_compile_and_deploy.sh — Compile gbp, then build the AppImage
# =============================================================================
#
#  Usage : ./gbp_linux_compile_and_deploy.sh x.y.z [manual | nomanual]
#          (e.g. ./gbp_linux_compile_and_deploy.sh 1.9.0)
#
#  No 2nd argument : both AppImages, the Embedded manuals version first
#  manual          : only the "Embedded manuals version", gbp-x.y.z-x86_64.AppImage
#  nomanual        : only the "Online manuals version", gbp-x.y.z-nomanual-x86_64.AppImage
#
#  For each version, runs gbp_linux_compile.sh, then, only if the compilation
#  succeeded, gbp_linux_deploy.sh with the version ("nomanual" is passed to both
#  for the Online manuals version). It stops at the first failure. Both scripts
#  must be in the same directory as this one. It can be run from any directory.
# =============================================================================

die() { echo "ERROR : $*" >&2; exit 1; }

USAGE="Usage : $0 x.y.z [manual | nomanual]  e.g. $0 1.9.0"
[[ $# -eq 1 || $# -eq 2 ]] || die "$USAGE"
# Check the version now (same rule as gbp_linux_deploy.sh), rather than after a long compilation
[[ "$1" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || die "invalid version \"$1\". Expected x.y.z  e.g. 1.9.0"

# Versions to build : "" is the Embedded manuals version, "nomanual" the Online manuals version
case "${2:-}" in
    "")       VARIANTS=("" "nomanual") ;;
    manual)   VARIANTS=("") ;;
    nomanual) VARIANTS=("nomanual") ;;
    *)        die "unknown argument \"$2\". $USAGE" ;;
esac

# Directory of this script : the other scripts are found from any current directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

for v in "${VARIANTS[@]}"; do
    # ${v:+"$v"} : passes "nomanual" only for the Online manuals version
    "$SCRIPT_DIR/gbp_linux_compile.sh" ${v:+"$v"} \
        || die "compilation failed${v:+ ($v)}, its AppImage is not built"
    "$SCRIPT_DIR/gbp_linux_deploy.sh" "$1" ${v:+"$v"} || die "deployment failed${v:+ ($v)}"
    echo "Release $1 ${v:+$v }done"
done
