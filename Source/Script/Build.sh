#!/usr/bin/env bash
set -ex
script_dir="$(dirname "$0")"
toplvl_dir="$(realpath "$script_dir/../../")"

build_dir="$toplvl_dir/Build"
build_config="${1:-Debug}"
install_dir="$toplvl_dir/Bin/$build_config"
generator="${2:-Unix Makefiles}"

mkdir -p "$build_dir"

pushd "$build_dir"

if [[ $(uname -s) = *MINGW64* ]]
then
	generator="MSYS Makefiles"
fi

cmake -S "$toplvl_dir" -B "$build_dir" -DCMAKE_BUILD_TYPE="$build_config" -G "$generator"
make -j$(nproc)

make install DESTDIR="$install_dir"

if [[ $(uname -s) = *MINGW64* ]]
then
	make bundle_dependencies
fi

popd
