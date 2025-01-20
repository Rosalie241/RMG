#!/usr/bin/env bash
set -ex

script_dir="$(dirname "$0")"
toplvl_dir="$(realpath "$script_dir/../../")"
bin_dir="$toplvl_dir/Bin/AppImage" # RMG should be installed here
UPINFO="gh-releases-zsync|$(echo "$GITHUB_REPOSITORY" | tr '/' '|')|latest|*.AppImage.zsync"
LIB4BN="https://raw.githubusercontent.com/VHSgunzo/sharun/refs/heads/main/lib4bin"
APPIMAGETOOL="https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage"

export ARCH="$(uname -m)"
export APPIMAGE_EXTRACT_AND_RUN=1
export VERSION="$(git describe --tags --always)"
export OUTPUT="$bin_dir/../RMG-Portable-Linux64-$VERSION.AppImage"
export LD_LIBRARY_PATH="$toplvl_dir/Build/AppImage/Source/RMG-Core" # hack

cp "$bin_dir"/usr/share/applications/com.github.Rosalie241.RMG.desktop "$bin_dir"
cp "$bin_dir"/usr/share/icons/hicolor/scalable/apps/com.github.Rosalie241.RMG.svg "$bin_dir"
ln -s ./com.github.Rosalie241.RMG.svg "$bin_dir"/.DirIcon
mv "$bin_dir"/usr/share "$bin_dir"/share
mv "$bin_dir"/usr "$bin_dir"/shared

if [ ! -f "./lib4bin" ]; then
	curl -L "$LIB4BN" -o ./lib4bin
	chmod +x ./lib4bin
fi

xvfb-run -a -- ./lib4bin --dst-dir "$bin_dir" -p -v -r -s -k -e \
	"$bin_dir"/shared/bin/RMG \
	/usr/lib/x86_64-linux-gnu/libSDL* \
	/usr/lib/x86_64-linux-gnu/libGL* \
	/usr/lib/x86_64-linux-gnu/libEGL* \
	/usr/lib/x86_64-linux-gnu/libvulkan* \
	/usr/lib/x86_64-linux-gnu/dri/* \
	/usr/lib/x86_64-linux-gnu/libssl.so* \
	/usr/lib/x86_64-linux-gnu/qt6/plugins/iconengines/* \
	/usr/lib/x86_64-linux-gnu/qt6/plugins/imageformats/* \
	/usr/lib/x86_64-linux-gnu/qt6/plugins/platforms/* \
	/usr/lib/x86_64-linux-gnu/qt6/plugins/platformthemes/* \
	/usr/lib/x86_64-linux-gnu/qt6/plugins/styles/* \
	/usr/lib/x86_64-linux-gnu/qt6/plugins/xcbglintegrations/* \
	/usr/lib/x86_64-linux-gnu/qt6/plugins/tls/* \
	/usr/lib/x86_64-linux-gnu/qt6/plugins/wayland-*/*

# Prepare sharun
"$bin_dir"/sharun -g
cp -v "$script_dir/AppRun" "$bin_dir"
chmod +x "$bin_dir"/AppRun

# make appimage
if [ ! -f "./appimagetool" ]; then
	curl -L "$APPIMAGETOOL" -o ./appimagetool
	chmod +x ./appimagetool
fi
./appimagetool --comp zstd \
	--mksquashfs-opt -Xcompression-level --mksquashfs-opt 22 \
	-n -u "$UPINFO" "$bin_dir" "$OUTPUT"
