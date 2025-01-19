#!/usr/bin/env bash
set -ex

script_dir="$(dirname "$0")"
toplvl_dir="$(realpath "$script_dir/../../")"
bin_dir="$toplvl_dir/Bin/AppImage" # RMG should be installed here
UPINFO="gh-releases-zsync|$(echo "$GITHUB_REPOSITORY" | tr '/' '|')|latest|*$ARCH.AppImage.zsync"
LIB4BN="https://raw.githubusercontent.com/VHSgunzo/sharun/refs/heads/main/lib4bin"
APPIMAGETOOL="https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage"

export ARCH="$(uname -m)"
export APPIMAGE_EXTRACT_AND_RUN=1
export VERSION="$(git describe --tags --always)"
export OUTPUT="$bin_dir/../RMG-Portable-Linux64-$VERSION.AppImage"
export LD_LIBRARY_PATH="$toplvl_dir/Build/AppImage/Source/RMG-Core" # hack

cd "$bin_dir"
cp ./usr/share/applications/com.github.Rosalie241.RMG.desktop ./
cp ./usr/share/icons/hicolor/scalable/apps/com.github.Rosalie241.RMG.svg ./
ln -s ./com.github.Rosalie241.RMG.svg ./.DirIcon
mv ./usr/share ./share
mv ./usr ./shared

if [ ! -f "./lib4bin" ]; then
	wget --retry-connrefused --tries=30 "$LIB4BN" -O ./lib4bin
	chmod +x ./lib4bin
fi

./lib4bin -p -v -r -s -k ./shared/bin/RMG \
	/usr/lib/x86_64-linux-gnu/libGL* \
	/usr/lib/x86_64-linux-gnu/libEGL* \
	/usr/lib/x86_64-linux-gnu/dri/* \
	/usr/lib/x86_64-linux-gnu/qt6/plugins/iconengines/* \
	/usr/lib/x86_64-linux-gnu/qt6/plugins/imageformats/* \
	/usr/lib/x86_64-linux-gnu/qt6/plugins/platforms/* \
	/usr/lib/x86_64-linux-gnu/qt6/plugins/platformthemes/* \
	/usr/lib/x86_64-linux-gnu/qt6/plugins/styles/* \
	/usr/lib/x86_64-linux-gnu/qt6/plugins/xcbglintegrations/* \
	/usr/lib/x86_64-linux-gnu/qt6/plugins/wayland-*/*

# Prepare sharun
./sharun -g
echo '#!/usr/bin/env sh
set -e
HERE="$(readlink -f "$(dirname "$0")")"
exec "$HERE/bin/RMG" \
	--lib-path="$HERE/shared/lib/RMG" \
	--core-path="$HERE/shared/lib/RMG/Core" \
	--plugin-path="$HERE/shared/lib/RMG/Plugin" \
	--shared-data-path="$HERE/share/RMG" \
	"${@}"' > ./AppRun

# make appimage
cd ..
if [ ! -f "./appimagetool" ]; then
	wget --retry-connrefused --tries=30 "$APPIMAGETOOL" -O ./appimagetool
	chmod +x ./appimagetool
fi
./appimagetool --comp zstd \
	--mksquashfs-opt -Xcompression-level --mksquashfs-opt 22 \
	-n -u "$UPINFO" "$PWD"/AppImage "$PWD"/RMG-"$VERSION"-anylinux-"$ARCH".AppImage
