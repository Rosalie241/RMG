#!/usr/bin/env bash
set -ex
script_dir="$(dirname "$0")"
toplvl_dir="$(realpath "$script_dir/../../")"

build_dir="$toplvl_dir/Build"
install_dir="$toplvl_dir/Bin"

mkdir -p "$build_dir"

pushd "$build_dir"

cmake -S "$toplvl_dir" -B "$build_dir"
make -j6

make install DESTDIR="$install_dir"

popd
