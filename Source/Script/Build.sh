#!/usr/bin/env bash
set -ex
script_dir="$(dirname "$0")"
toplvl_dir="$(realpath "$script_dir/../../")"
build_config="${1:-Debug}"
build_dir="$toplvl_dir/Build/$build_config"
install_dir="$toplvl_dir/Bin/$build_config"
threads="${2:-$(nproc)}"

generator="Unix Makefiles"
msys2="0"

if [ "$1" = "--help" ] ||
    [ "$1" = "-h" ]
then
    echo "$0 [Build Config] [Thread Count]"
    exit
fi

mkdir -p "$build_dir"

pushd "$build_dir"

if [[ $(uname -s) = *MINGW64* ]]
then
    msys2="1"
    generator="MSYS Makefiles"
fi

cmake -S "$toplvl_dir" -B "$build_dir" -DCMAKE_BUILD_TYPE="$build_config" -DPORTABLE_INSTALL="${RMG_PORTABLE:-ON}" -G "$generator" -DCMAKE_EXPORT_COMPILE_COMMANDS=1

make install DESTDIR="$toplvl_dir" -j$threads

if [[ "$msys2" = "1" ]]
then
    make bundle_dependencies
fi

popd
