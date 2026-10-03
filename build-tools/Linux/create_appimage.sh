#!/bin/bash

# Version of this build script (bump when the plugin-trimming logic below changes).
SCRIPT_VERSION="1.1.0"

# Usage: ./create_appimage.sh <gbp-version>   (e.g. ./create_appimage.sh 1.8.2)
GBP_VERSION="$1"
if [ -z "$GBP_VERSION" ]; then
    echo "Usage: $0 <gbp-version>   (e.g. $0 1.8.2)" >&2
    exit 1
fi
APPIMAGE_FILE="gbp-$GBP_VERSION-x86_64.AppImage"

# Define Qt version once
QT_VERSION=6.10.3
QT_PATH=$HOME/Qt/$QT_VERSION/gcc_64

# Clean up
rm -rf AppDir

# Set environment variables
export LD_LIBRARY_PATH=$QT_PATH/lib
export PATH=$QT_PATH/bin:$PATH
export LDAI_RUNTIME_FILE=$(pwd)/runtime-x86_64
# Output file name for linuxdeploy-plugin-appimage (otherwise derived from the desktop file's Name)
export LDAI_OUTPUT="$APPIMAGE_FILE"


echo "create_appimage.sh version: $SCRIPT_VERSION"
echo "GBP Version: $GBP_VERSION"
echo "Qt Version: $QT_VERSION"
echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
echo "PATH=$PATH"

# First linuxdeploy run
./linuxdeploy-x86_64.AppImage \
    --appdir=AppDir \
    --executable=./graphical-budget-planner \
    --plugin=qt \
    --desktop-file=graphical-budget-planner.desktop \
    --icon-file=graphical-budget-planner.png

# Copy translations
cp -v *.qm AppDir/usr/bin || true

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
cp -v $QT_PATH/lib/libQt6WaylandClient.so.* AppDir/usr/lib/ || true
cp -v $QT_PATH/lib/libQt6WaylandEglClientHwIntegration.so.* AppDir/usr/lib/ || true

# --- 🔥 Cleanup: remove system GTK/GLib that break qgtk3 ---
for lib in libglib-2.0 libgobject-2.0 libgio-2.0 libgmodule-2.0 libgtk-3 libgdk-3; do
    rm -f AppDir/usr/lib/${lib}.so* || true
done

# Final AppImage build
./linuxdeploy-x86_64.AppImage --appdir=AppDir --output=appimage

echo "✅ AppImage build complete."

# Report the size of the AppImage that was just produced
if [ -f "$APPIMAGE_FILE" ]; then
    echo "AppImage size: $(du -h "$APPIMAGE_FILE" | cut -f1)  ($APPIMAGE_FILE)"
else
    echo "WARNING: expected output $APPIMAGE_FILE was not found."
fi

