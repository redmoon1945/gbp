#!/bin/bash

# Define Qt version once
QT_VERSION=6.10.3
QT_PATH=$HOME/Qt/$QT_VERSION/gcc_64

# Clean up
rm -rf AppDir

# Set environment variables
export LD_LIBRARY_PATH=$QT_PATH/lib
export PATH=$QT_PATH/bin:$PATH
export LDAI_RUNTIME_FILE=$(pwd)/runtime-x86_64


echo "Qt Version: $QT_VERSION"
echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
echo "PATH=$PATH"

# First linuxdeploy run
./linuxdeploy-x86_64.AppImage \
    --appdir=AppDir \
    --executable=./gbp \
    --plugin=qt \
    --desktop-file=gbp.desktop \
    --icon-file=gbp.png

# Copy translations
cp -v *.qm AppDir/usr/bin || true

# ---- Ensure Qt plugins are included ----
PLUGINS_SRC=$QT_PATH/plugins
PLUGINS_DST=AppDir/usr/plugins

for plugin_dir in platforms platformthemes imageformats styles bearer generic iconengines inputmethods tls wayland-shell-integration; do
    if [ -d "$PLUGINS_SRC/$plugin_dir" ]; then
        mkdir -p "$PLUGINS_DST/$plugin_dir"
        cp -v "$PLUGINS_SRC/$plugin_dir"/*so "$PLUGINS_DST/$plugin_dir/" || true
    fi
done

# Copy missing Wayland-related Qt libs
cp -v $QT_PATH/lib/libQt6WaylandClient.so.* AppDir/usr/lib/ || true
cp -v $QT_PATH/lib/libQt6WaylandEglClientHwIntegration.so.* AppDir/usr/lib/ || true
cp -v $QT_PATH/lib/libQt6WaylandCompositor.so.* AppDir/usr/lib/ || true

# --- 🔥 Cleanup: remove system GTK/GLib that break qgtk3 ---
for lib in libglib-2.0 libgobject-2.0 libgio-2.0 libgmodule-2.0 libgtk-3 libgdk-3; do
    rm -f AppDir/usr/lib/${lib}.so* || true
done

# Final AppImage build
./linuxdeploy-x86_64.AppImage --appdir=AppDir --output=appimage

echo "✅ AppImage build complete."

