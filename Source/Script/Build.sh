#!/usr/bin/env bash
set -ex
script_dir="$(dirname "$0")"
toplvl_dir="$(realpath "$script_dir/../../")"
build_config="${1:-Debug}"
build_dir="$toplvl_dir/Build/$build_config"
threads="${2:-$(nproc)}"

if [ "$1" = "--help" ] ||
    [ "$1" = "-h" ]
then
    echo "$0 [Build Config] [Thread Count]"
    exit
fi

mkdir -p "$build_dir"

cmake -S "$toplvl_dir" -B "$build_dir" -DCMAKE_BUILD_TYPE="$build_config" -DPORTABLE_INSTALL=ON -DUSE_ANGRYLION=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G "Ninja"

cmake --build "$build_dir" --parallel "$threads"

if [[ "$build_config" = "Debug" ]]
then
    cmake --install "$build_dir" --prefix="$toplvl_dir"
else
    cmake --install "$build_dir" --strip --prefix="$toplvl_dir"
fi

if [[ $(uname -s) = *MINGW64* ]]
then
    cmake --build "$build_dir" --target=bundle_dependencies
fi
