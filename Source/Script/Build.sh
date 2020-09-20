#!/usr/bin/env bash
set -ex
script_dir="$(dirname "$0")"
toplvl_dir="$(realpath "$script_dir/../../")"

build_dir="$toplvl_dir/Build"
build_config="${1:-Debug}"
install_dir="$toplvl_dir/Bin/$build_config"

mkdir -p "$build_dir"

pushd "$build_dir"

cmake -S "$toplvl_dir" -B "$build_dir" -DCMAKE_BUILD_TYPE="$build_config"
make -j6

make install DESTDIR="$install_dir"

popd
