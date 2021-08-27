#!/usr/bin/env bash
set -ex
script_dir="$(dirname "$0")"
toplvl_dir="$(realpath "$script_dir/../../")"
build_config="${1:-Debug}"
version="$(git describe --always)"

pushd "$toplvl_dir/Bin/$build_config"

zip -r "$toplvl_dir/Bin/RMG-linux-$version.zip" *

popd
