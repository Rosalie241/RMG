#!/usr/bin/env bash
set -ex

script_dir="$(dirname "$0")"
toplvl_dir="$(realpath "$script_dir/../../")"
bin_dir="$toplvl_dir/Bin/AppImage" # RMG should be installed here

export QMAKE="$(which qmake6)"
export EXTRA_QT_PLUGINS="imageformats;iconengines;"
export VERSION="$(git describe --tags --always)"
export OUTPUT="$bin_dir/../RMG-Portable-Linux64-$VERSION.AppImage"
export LD_LIBRARY_PATH="$toplvl_dir/Build/AppImage/Source/RMG-Core" # hack

"$(pwd)/squashfs-root/AppRun" \
    --plugin=qt \
    --appdir="$bin_dir" \
    --custom-apprun="$script_dir/AppRun" \
    --output=appimage \
    --desktop-file="$bin_dir/usr/share/applications/com.github.Rosalie241.RMG.desktop"
