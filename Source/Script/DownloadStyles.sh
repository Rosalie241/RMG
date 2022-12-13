#!/usr/bin/env bash
set -e
script_dir="$(dirname "$0")"
toplvl_dir="$(realpath "$script_dir/../../")"
tmp_dir="$toplvl_dir/Build/RMG-Dark-Style/"
target_styles_dir="$toplvl_dir/Bin/Release/Data/Styles/"

mkdir -p "$target_styles_dir"
rm -rf "$tmp_dir"

git clone https://github.com/Rosalie241/Dolphin-Dark-Mode/ "$tmp_dir"
pushd "$tmp_dir"
git checkout -f cb07df26c49408a3e1122489d5a66ce2c70963fd
popd

cp -r "$tmp_dir/Data/Styles"/* "$target_styles_dir/"
