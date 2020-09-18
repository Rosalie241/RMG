#!/usr/bin/env bash
set -e

script_dir="$(dirname "$0")"
source_dir="$(realpath "$script_dir/../")"

out_file="$source_dir/RMG/Res/Icon256.png"
src_file="$source_dir/../Artwork/RMG Icon.xcf"

echo "Generating $out_file"
convert -layers merge "$src_file" "$out_file"
convert -transparent white -resize 256x256 "$out_file" "$out_file"

