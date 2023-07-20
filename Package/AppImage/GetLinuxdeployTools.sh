#!/usr/bin/env bash
set -ex

script_dir="$(realpath $(dirname "$0"))"
toplvl_dir="$(realpath "$script_dir/../../")"

# A script that needs to be run once to get the linuxdeploy tools
# that are used to create AppImages. The .github/workflows/build.yml
# does this setup in the Install Packages step on the build server,
# but this must be run before building an appimage on a local machine.

if [ ! -f "$script_dir/linuxdeploy-x86_64.AppImage" ]
then
    curl -L https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage \
        -o "$script_dir/linuxdeploy-x86_64.AppImage"
    chmod +x "$script_dir/linuxdeploy-x86_64.AppImage"
fi

if [ ! -f "$script_dir/linuxdeploy-plugin-qt-x86_64.AppImage" ]
then
    curl -L https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage \
        -o "$script_dir/linuxdeploy-plugin-qt-x86_64.AppImage"
    chmod +x "$script_dir/linuxdeploy-plugin-qt-x86_64.AppImage"
fi

# To mirror .github/workflows/build.yml this script will always unpack
# the linuxdeploy tools to $toplvl_dir/squashfs
pushd "$toplvl_dir"
"$script_dir/linuxdeploy-plugin-qt-x86_64.AppImage" --appimage-extract
"$script_dir/linuxdeploy-x86_64.AppImage" --appimage-extract
popd

# delete appimages
rm "$script_dir/linuxdeploy-x86_64.AppImage" \
    "$script_dir/linuxdeploy-plugin-qt-x86_64.AppImage"

