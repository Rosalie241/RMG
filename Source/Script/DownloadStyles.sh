#!/usr/bin/env bash
set -ex
script_dir="$(dirname "$0")"
toplvl_dir="$(realpath "$script_dir/../../")"
tmp_dir="$toplvl_dir/Build/RMG-Dark-Style/"
target_styles_dir="$toplvl_dir/Bin/Release/Data/Styles/"

mkdir -p "$target_styles_dir"
rm -rf "$tmp_dir"

git clone https://github.com/Rosalie241/Dolphin-Dark-Mode/ "$tmp_dir" -b RMG

cp -r "$tmp_dir/Data/Styles"/* "$target_styles_dir/"
