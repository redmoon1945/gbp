#!/bin/bash

# Version of this build script (bump when the plugin-trimming logic below changes).
SCRIPT_VERSION="1.3.0"

# Usage: ./gbp_linux_deploy.sh <gbp-version> [nomanual]   (e.g. ./gbp_linux_deploy.sh 1.9.0)
#
# Without "nomanual" : "Embedded manuals version", gbp-<version>-x86_64.AppImage
# With "nomanual"    : "Online manuals version", gbp-<version>-nomanual-x86_64.AppImage, made from
#                      the build of "gbp_linux_compile.sh nomanual" (own BUILD_DIR)
#
# Builds the AppImage in WORK_DIR (GBP_APP_IMAGE_BUILD_DIR in BUILDING.md), from :
#   - the executable and the translation files (.qm) compiled by gbp_linux_compile.sh, taken
#     from BUILD_DIR
#   - the desktop file and the icon, taken from the gbp sources (REPO_DIR)
#   - the 4 tools downloaded once in WORK_DIR : linuxdeploy-x86_64.AppImage,
#     linuxdeploy-plugin-qt-x86_64.AppImage, appimagetool-x86_64.AppImage and runtime-x86_64
# It can be run from any directory. The directories below can be changed here, or overridden
# for one run from the command line, e.g. :  WORK_DIR=/tmp/appimage ./gbp_linux_deploy.sh 1.9.0

die() { echo "ERROR : $*" >&2; exit 1; }

GBP_VERSION="$1"
if [ -z "$GBP_VERSION" ] || [ $# -gt 2 ]; then
    echo "Usage: $0 <gbp-version> [nomanual]   (e.g. $0 1.9.0)" >&2
    exit 1
fi
[[ "$GBP_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || die "invalid version \"$GBP_VERSION\". Expected x.y.z  e.g. 1.9.0"
case "$2" in
    "")       EMBEDDED_MANUALS=ON;  SUFFIX="" ;;
    nomanual) EMBEDDED_MANUALS=OFF; SUFFIX="-nomanual" ;;
    *)        die "unknown argument \"$2\". Usage: $0 <gbp-version> [nomanual]" ;;
esac
APPIMAGE_FILE="gbp-$GBP_VERSION$SUFFIX-x86_64.AppImage"

# Define Qt version once
QT_VERSION="${QT_VERSION:-6.11.3}"
# Same version with "_" instead of "." (e.g. 6_11_3), as in the build folder name
QT_VERSION_U="${QT_VERSION//./_}"
QT_PATH="${QT_PATH:-$HOME/Qt/$QT_VERSION/gcc_64}"
# Same defaults as gbp_linux_compile.sh
REPO_DIR="${REPO_DIR:-$HOME/data/dev/Qt/gbp}"
BUILD_DIR="${BUILD_DIR:-$HOME/data/dev/Qt/build/gbp/Desktop_Qt_${QT_VERSION_U}-Release$SUFFIX}"
WORK_DIR="${WORK_DIR:-$HOME/gbp-app-image-build}"

# Every file needed must be there : stop now rather than build an incomplete AppImage
EXE_FILE="$BUILD_DIR/graphical-budget-planner"
DESKTOP_FILE="$REPO_DIR/build-tools/Linux/graphical-budget-planner.desktop"
ICON_FILE="$REPO_DIR/build-tools/graphical-budget-planner.png"
[ -d "$QT_PATH/lib" ]     || die "Qt $QT_VERSION not found in $QT_PATH (set QT_PATH)"
[ -x "$EXE_FILE" ]        || die "$EXE_FILE not found : run gbp_linux_compile.sh${2:+ $2} first"
# The executable must be the requested version (embedded or online manuals) : never ship one
# under the name of the other
grep -qx "GBP_EMBEDDED_MANUALS:BOOL=$EMBEDDED_MANUALS" "$BUILD_DIR/CMakeCache.txt" 2>/dev/null \
    || die "$BUILD_DIR was not built with GBP_EMBEDDED_MANUALS=$EMBEDDED_MANUALS :" \
           "run gbp_linux_compile.sh${2:+ $2} first"
[ -f "$BUILD_DIR/gbp_en.qm" ] && [ -f "$BUILD_DIR/gbp_fr.qm" ] \
                          || die "translation files gbp_en.qm / gbp_fr.qm not found in $BUILD_DIR"
[ -f "$DESKTOP_FILE" ]    || die "$DESKTOP_FILE not found (set REPO_DIR)"
[ -f "$ICON_FILE" ]       || die "$ICON_FILE not found (set REPO_DIR)"
[ -d "$WORK_DIR" ]        || die "$WORK_DIR not found : create it and put the 4 downloaded tools in it"
cd "$WORK_DIR" || die "cannot go to $WORK_DIR"
for tool in linuxdeploy-x86_64.AppImage linuxdeploy-plugin-qt-x86_64.AppImage \
            appimagetool-x86_64.AppImage; do
    [ -x "$tool" ] || die "$WORK_DIR/$tool not found or not executable (chmod a+x *.AppImage)"
done
[ -f runtime-x86_64 ]     || die "$WORK_DIR/runtime-x86_64 not found"

# Clean up
rm -rf AppDir

# Set environment variables
export LD_LIBRARY_PATH=$QT_PATH/lib
export PATH=$QT_PATH/bin:$PATH


echo "gbp_linux_deploy.sh version: $SCRIPT_VERSION"
echo "GBP Version: $GBP_VERSION"
echo "Embedded manuals: $EMBEDDED_MANUALS"
echo "Qt Version: $QT_VERSION"
echo "Executable: $EXE_FILE"
echo "Work dir: $WORK_DIR"
echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
echo "PATH=$PATH"

# First linuxdeploy run
./linuxdeploy-x86_64.AppImage \
    --appdir=AppDir \
    --executable="$EXE_FILE" \
    --plugin=qt \
    --desktop-file="$DESKTOP_FILE" \
    --icon-file="$ICON_FILE" \
    || die "first linuxdeploy run failed"

# Copy translations
cp -v "$BUILD_DIR"/gbp_*.qm AppDir/usr/bin || die "cannot copy the translation files"

# Qt's own translations (qtbase_*.qm, deployed by linuxdeploy-plugin-qt in 33 languages) : keep
# only English and French, the 2 languages of GBP (saves about 0.7 MB)
find AppDir/usr/translations -name '*.qm' ! -name 'qtbase_en.qm' ! -name 'qtbase_fr.qm' \
    -delete || true

# ---- Ensure Qt plugins are included ----
# GBP only links Qt6::Widgets + Qt6::Charts (see src/CMakeLists.txt) -- no Network, Sql,
# PrintSupport, Multimedia, Qml/Quick or Svg. main.cpp's own session-detection logic (search
# for "XWaylandBehavior" / "xdgdesktopportal") is what actually drives platform selection:
# it forces xcb on GNOME/Cinnamon Wayland, supports native Wayland elsewhere, and sets the
# xdgdesktopportal theme on Wayland. So only these plugin categories are kept at all:
#   - platforms          (xcb + wayland QPA backends -- both are genuinely used, see above)
#   - platformthemes     (xdgdesktopportal is set explicitly for Wayland; qgtk3 covers X11/
#                          XWayland desktops that set QT_QPA_PLATFORMTHEME=gtk3 themselves)
#   - wayland-shell-integration (xdg-shell only -- the protocol every real compositor speaks)
# Everything else that used to be force-copied here (imageformats, iconengines, tls, generic,
# inputmethods, styles, bearer) is unused by GBP: no image formats beyond built-in PNG are
# loaded, no QtNetwork/QSslSocket exists to need a TLS backend, no SVG icons, no touch/evdev
# input, and "bearer" was removed from Qt6 entirely (this loop always no-op'd on it).
PLUGINS_SRC=$QT_PATH/plugins
PLUGINS_DST=AppDir/usr/plugins

for plugin_dir in platforms platformthemes wayland-shell-integration; do
    if [ -d "$PLUGINS_SRC/$plugin_dir" ]; then
        mkdir -p "$PLUGINS_DST/$plugin_dir"
        cp -v "$PLUGINS_SRC/$plugin_dir"/*so "$PLUGINS_DST/$plugin_dir/" || true
    fi
done

# Trim platforms/ down to the two QPA backends GBP actually uses (xcb + wayland).
# Drops the embedded/headless backends (eglfs, linuxfb, minimal, minimalegl, offscreen,
# vkkhrdisplay, vnc) that a normally-launched desktop app never loads.
find "$PLUGINS_DST/platforms" -type f \
    ! -name 'libqxcb.so' ! -name 'libqwayland*.so' -delete || true

# Trim wayland-shell-integration/ down to xdg-shell (the protocol GNOME/KDE/Sway/etc. all
# speak). Drops ivi-shell/layer-shell/fullscreen-shell/wl-shell/qt-shell, which only matter
# for kiosk/embedded/legacy compositors GBP doesn't target.
find "$PLUGINS_DST/wayland-shell-integration" -type f \
    ! -name 'libxdg-shell.so' -delete || true

# Belt-and-suspenders: linuxdeploy's own --plugin=qt pass (above) bundles some plugin
# categories by default heuristics regardless of this script's own copy loop. Explicitly
# strip anything unused that may have landed there anyway.
rm -rfv "$PLUGINS_DST"/{imageformats,iconengines,tls,generic,inputmethods,styles} || true

# IMPORTANT: linuxdeploy-plugin-qt also always bundles platforminputcontexts/ by default,
# including libqtvirtualkeyboardplugin.so (an on-screen keyboard plugin for touchscreens --
# irrelevant to a desktop app with a physical keyboard). That one plugin alone drags in a huge
# unwanted chain: Qt6Quick, Qt6Qml(+Meta/Models/WorkerScript), Qt6VirtualKeyboard(Qml),
# Qt6Network, Qt6Svg, and transitively libkrb5/libgssapi_krb5/libk5crypto/libkrb5support/
# libkeyutils (Qt6Network's GSSAPI auth support) -- confirmed by inspecting an actual build
# log, none of which GBP uses. Only this one plugin is removed: libibusplatforminputcontextplugin.so
# and libcomposeplatforminputcontextplugin.so (the other two files in this directory) are kept,
# since they provide real IME / compose-key support for accented and CJK text entry and add no
# extra heavy dependencies of their own.
rm -fv "$PLUGINS_DST/platforminputcontexts/libqtvirtualkeyboardplugin.so" || true

# Deleting the plugins above is NOT enough on its own: linuxdeploy's final pass re-scans every
# file *physically present* in AppDir/usr/lib and AppDir/usr/plugins and redeploys whatever ELF
# dependencies it declares -- it has no notion of "the plugin that needed this is gone, so this
# is now an orphan". Confirmed by an actual build: after removing only the plugin files above,
# the final AppImage came out the exact same size (60.32 MB, one file fewer), because the
# libraries those plugins had already pulled into AppDir/usr/lib during the first pass were
# still sitting there, and the final pass re-resolved their own dependency chains to satisfy
# them. So every leaf library that existed ONLY to back a plugin we removed must be deleted
# here too:
#   - libQt6VirtualKeyboard(Qml)/Quick/Qml(Meta/Models/WorkerScript)/Network/Svg: only pulled in
#     by the deleted platforminputcontexts/libqtvirtualkeyboardplugin.so
#   - libkrb5/libgssapi_krb5/libk5crypto/libkrb5support/libkeyutils: only pulled in transitively
#     by libQt6Network's GSSAPI/Negotiate auth support, now itself removed
#   - libtiff/libwebp/libjbig/libjpeg/libdeflate: only pulled in by the deleted imageformats
#     plugins (libqtiff.so/libqwebp.so), confirmed still present in AppDir/usr/lib after that
#     plugin directory was removed
rm -fv AppDir/usr/lib/libQt6VirtualKeyboard.so* \
       AppDir/usr/lib/libQt6VirtualKeyboardQml.so* \
       AppDir/usr/lib/libQt6Quick.so* \
       AppDir/usr/lib/libQt6Qml.so* \
       AppDir/usr/lib/libQt6QmlMeta.so* \
       AppDir/usr/lib/libQt6QmlModels.so* \
       AppDir/usr/lib/libQt6QmlWorkerScript.so* \
       AppDir/usr/lib/libQt6Network.so* \
       AppDir/usr/lib/libQt6Svg.so* \
       AppDir/usr/lib/libgssapi_krb5.so* \
       AppDir/usr/lib/libkrb5.so* \
       AppDir/usr/lib/libk5crypto.so* \
       AppDir/usr/lib/libkrb5support.so* \
       AppDir/usr/lib/libkeyutils.so* \
       AppDir/usr/lib/libtiff.so* \
       AppDir/usr/lib/libwebp.so* \
       AppDir/usr/lib/libjbig.so* \
       AppDir/usr/lib/libjpeg.so* \
       AppDir/usr/lib/libdeflate.so* \
       || true

# Copy missing Wayland-related Qt libs. Compositor lib is NOT included: that's for an app
# that *implements* a Wayland compositor (a window manager) -- GBP is a normal client window.
# Only the ".so.6" name is copied (-L : the file, not the link), the one the plugins ask for :
# copying ".so.*" put the same 1.2 MB library twice in the AppImage (".so.6" and ".so.6.x.y").
cp -vL $QT_PATH/lib/libQt6WaylandClient.so.6 AppDir/usr/lib/ || true
cp -vL $QT_PATH/lib/libQt6WaylandEglClientHwIntegration.so.6 AppDir/usr/lib/ || true

# --- 🔥 Cleanup: remove system GTK/GLib that break qgtk3 ---
for lib in libglib-2.0 libgobject-2.0 libgio-2.0 libgmodule-2.0 libgtk-3 libgdk-3; do
    rm -f AppDir/usr/lib/${lib}.so* || true
done

# Final linuxdeploy run : deploys the dependencies of the files added above, without building
# the AppImage (linuxdeploy cannot set the compression level : see below)
./linuxdeploy-x86_64.AppImage --appdir=AppDir || die "final linuxdeploy run failed"

# Build the AppImage with appimagetool, called directly with the static runtime runtime-x86_64
# (see the Appendix of BUILDING.md). By default (also through linuxdeploy), the squashfs is
# compressed with zstd at the default level in blocks of 128 KB. Larger blocks (1 MB) and level
# 19 make the AppImage about 3 MB smaller, without slowing down the start of GBP (zstd
# decompression speed hardly depends on the level). The "-b 1M" given here comes after the
# "-b 128K" of appimagetool : mksquashfs keeps the last one.
ARCH=x86_64 ./appimagetool-x86_64.AppImage \
    --runtime-file "$(pwd)/runtime-x86_64" \
    --comp zstd \
    --mksquashfs-opt=-b --mksquashfs-opt=1M \
    --mksquashfs-opt=-Xcompression-level --mksquashfs-opt=19 \
    AppDir "$APPIMAGE_FILE" \
    || die "appimagetool failed (AppDir kept)"

echo "✅ AppImage build complete."

# Report the size of the AppImage that was just produced. Without it, stop with an error and
# keep AppDir, so that its content can be inspected.
[ -f "$APPIMAGE_FILE" ] || die "expected output $APPIMAGE_FILE was not found (AppDir kept)"
echo "AppImage size: $(du -h "$APPIMAGE_FILE" | cut -f1)  ($APPIMAGE_FILE)"

# AppDir was only the staging directory of the AppImage : no longer needed. (To look at what
# went into the AppImage, use :  ./gbp-x.y.z-x86_64.AppImage --appimage-extract)
rm -rf AppDir

