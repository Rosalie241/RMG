#!/usr/bin/env bash
set -e
script_dir="$(dirname "$0")"
toplvl_dir="$(realpath "$script_dir/../../")"
tmp_dir="$toplvl_dir/Build/project64"
source_cheat_dir="$tmp_dir/Config/Cheats"
target_cheat_dir="$toplvl_dir/Data/Cheats/Generated"

# remove generated cheats & tmp dir
rm -rf "$tmp_dir" "$target_cheat_dir"/*.cht
git clone https://github.com/project64/project64 "$tmp_dir"
cd "$tmp_dir"

# latest commit of cheats change
git checkout 1bde8589e95478a7e82bfa67e84c3face0802e1b -f

for file in "$source_cheat_dir"/*.cht
do
    target_file_name=""

    # read file line by line
    while read -r line
    do
        if [[ $line = "["* ]] && [[ $line = *"]" ]]
        then
            target_file_name="$(echo "$line" | sed 's|\[||g' | sed 's|\]||g' | sed 's|C:||g').cht"
            target_file_name="$target_cheat_dir/$target_file_name"

            # if file already exists,
            # just write to /dev/null
            # to prevent failures due to i.e https://github.com/project64/project64/pull/2312
            if [ -f "$target_file_name" ]
            then
                target_file_name=""
            fi

            # also account for invalid headers,
            # some cheat files have only 1 number 
            # in the country code, and there are exactly
            # the same cheat files with the correct length
            # so just skip them
            # i.e [A3A044B5-6DB1BF5E-C:0] and [A3A044B5-6DB1BF5E-C:00]
            if [[ "${#line}" != "24" ]]
            then
                target_file_name=""
            fi

            if [[ "x$target_file_name" != "x" ]]
            then
                echo "-> Generating $target_file_name"
            fi
        fi

        # sanity check
        if [[ "x$target_file_name" = "x" ]]
        then
            continue
        fi

        # write line to file
        echo "$line" >> "$target_file_name"
    done < "$file"
done

rm -rf "$tmp_dir"
