#!/usr/bin/env bash
set -ex
script_dir="$(dirname "$0")"
toplvl_dir="$(realpath "$script_dir/../../")"

pushd "$toplvl_dir"

find Source/ -iname *.hpp -o -iname *.cpp | xargs clang-format -i

popd