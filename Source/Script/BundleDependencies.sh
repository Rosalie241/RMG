#!/usr/bin/env bash
#
# ./BundleDependencies.sh "./Bin/Release/rmg/RMG.exe" "./Bin/Release/rmg" "/mingw64/bin"
#

exe="$1"
bin_dir="$2"
path="$3"

function copyForOBJ() {
    local deps=`objdump.exe -p $1 | grep 'DLL Name:' | sed -e "s/\t*DLL Name: //g"`
    while read -r line
	do
        findAndCopyDLL $line
    done <<< "$deps"
}

function findAndCopyDLL() {
    local file="$path/$1"
    
	if [ -f $file ] && [ ! -f "$bin_dir/$1" ]
	then
		cp "$file" "$bin_dir"
        copyForOBJ $file
		return 0
	fi

    return 0
}


for ext in dll exe
do
	while read -r file_line
	do
		echo "=> Copying dependencies for $file_line"
		copyForOBJ "$file_line"
	done < <(find "$bin_dir" -name "*.$ext")
done

windeployqt "$exe"

# needed by Qt at runtime
cp "$path/libcrypto-1_1-x64.dll" "$bin_dir/"
cp "$path/libssl-1_1-x64.dll"    "$bin_dir/"

exit 0
