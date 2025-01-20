#!/usr/bin/env bash
set -ex

script_dir="$(dirname "$0")"
toplvl_dir="$(realpath "$script_dir/../../")"
bin_dir="$toplvl_dir/Bin/AppImage" # RMG should be installed here
lib_dir="/usr/lib64"

XVFB_RUN="xvfb-run -a --"
LIB4BIN_URL="https://raw.githubusercontent.com/VHSgunzo/sharun/refs/heads/main/lib4bin"
APPIMAGETOOL_URL="https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage"
SHARUN_URL="https://github.com/VHSgunzo/sharun/releases/latest/download/sharun-x86_64"

export ARCH="$(uname -m)"
export APPIMAGE_EXTRACT_AND_RUN=1
export VERSION="$(git describe --tags --always)"
export OUTPUT="$bin_dir/../RMG-Portable-Linux64-$VERSION.AppImage"
export LD_LIBRARY_PATH="$toplvl_dir/Build/AppImage/Source/RMG-Core" # hack
export SHARUN="$script_dir/sharun"

if [[ ! -f "$script_dir/lib4bin" ]]
then
	curl -L "$LIB4BIN_URL" -o "$script_dir/lib4bin"
	chmod +x "$script_dir/lib4bin"
fi

if [[ ! -f "$script_dir/appimagetool" ]]
then
	curl -L "$APPIMAGETOOL_URL" -o "$script_dir/appimagetool"
	chmod +x "$script_dir/appimagetool"
fi

if [[ ! -f "$script_dir/sharun" ]]
then
	curl -L "$SHARUN_URL" -o "$script_dir/sharun"
	chmod +x "$script_dir/sharun"
fi

if [[ "x$DISPLAY" != "x" ]]
then
	XVFB_RUN=""
fi

if [[ -d "/usr/lib/x86_64-linux-gnu/" ]]
then
	lib_dir="/usr/lib/x86_64-linux-gnu/"
fi

cp "$bin_dir/usr/share/applications/com.github.Rosalie241.RMG.desktop" "$bin_dir"
cp "$bin_dir/usr/share/icons/hicolor/scalable/apps/com.github.Rosalie241.RMG.svg" "$bin_dir"
ln -s ./com.github.Rosalie241.RMG.svg "$bin_dir"/.DirIcon
mv "$bin_dir/usr/share" "$bin_dir/share"
mv "$bin_dir/usr" "$bin_dir/shared"

$XVFB_RUN "$script_dir/lib4bin" --dst-dir "$bin_dir" \
	--hard-links --patch-rpath --strip \
	--with-hooks --strace-mode --with-sharun \
	"$bin_dir/shared/bin/RMG" \
	"$lib_dir"/libSDL* \
	"$lib_dir"/libGL* \
	"$lib_dir"/libEGL* \
	"$lib_dir"/libvulkan* \
	"$lib_dir"/dri/* \
	"$lib_dir"/libssl.so* \
	"$lib_dir"/qt6/plugins/iconengines/* \
	"$lib_dir"/qt6/plugins/imageformats/* \
	"$lib_dir"/qt6/plugins/platforms/* \
	"$lib_dir"/qt6/plugins/platformthemes/* \
	"$lib_dir"/qt6/plugins/styles/* \
	"$lib_dir"/qt6/plugins/xcbglintegrations/* \
	"$lib_dir"/qt6/plugins/tls/* \
	"$lib_dir"/qt6/plugins/wayland-*/*

# Prepare sharun
"$bin_dir/sharun" -g
cp "$script_dir/AppRun" "$bin_dir"

# make appimage
"$script_dir/appimagetool" --comp zstd \
	--mksquashfs-opt -Xcompression-level --mksquashfs-opt 22 \
	-n "$bin_dir" "$OUTPUT"
