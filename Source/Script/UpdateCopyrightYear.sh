#!/usr/bin/env bash
set -ex
script_dir="$(dirname "$0")"
toplvl_dir="$(realpath "$script_dir/../../")"

curr_year=$(date "+%Y")
prev_year=$(($curr_year - 1))

dirs=(
	"$toplvl_dir/Source/RMG-Core"
	"$toplvl_dir/Source/RMG"
	"$toplvl_dir/Source/RMG-Input"
	"$toplvl_dir/Source/RMG-Audio"
)

for dir in ${dirs[@]}
do
	if [[ "$(git diff --name-only "$dir" | wc -l)" != "0" ]]
	then
		echo "$dir cannot contain uncommitted changes!"
		exit 1
	fi
done

for dir in ${dirs[@]}
do
	find "$dir" -type f \
		-exec sed -i "s|Copyright (C) 2020-$prev_year|Copyright (C) 2020-$curr_year|g" {} +

	if [[ "$(git diff --name-only "$dir" | wc -l)" != "0" ]]
	then
		git add -u "$dir"
		git commit -m "$(basename "$dir"): update copyright year"
	fi
done
