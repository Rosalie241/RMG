#!/usr/bin/env bash
set -ex
script_dir="$(dirname "$0")"
toplvl_dir="$(realpath "$script_dir/../../")"
m64psrc_dir="$toplvl_dir/Build/Release/Source/3rdParty/mupen64plus-core/src"
header_dir="$toplvl_dir/Source/RMG/M64P/api"

for h in common config debugger frontend plugin types vidext
do
    cp "$m64psrc_dir/api/m64p_$h.h" \
        "$header_dir"
done

cp "$m64psrc_dir/main/version.h" \
    "$header_dir"
