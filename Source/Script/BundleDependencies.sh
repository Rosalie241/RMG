#!/usr/bin/env bash
#
# ./BundleDependencies.sh "./Bin/Release/RMG.exe" "./Bin/Release/"
#

exe="$1"
bin_dir="$2"
path="/$MSYSTEM/bin"

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


for file in "$bin_dir"/*.exe "$bin_dir/Core"/*.dll "$bin_dir/Plugin"/*/*.dll
do
	echo "=> Copying dependencies for $file"
	copyForOBJ "$file"
done

windeployqt-qt6 --exclude-plugins qpdf,qwebp,qgif,qtga,qtuiotouchplugin,qglib,qtiff,qmng,qwbmp \
				--no-translations "$exe"

# needed by Qt at runtime
cp "$path/libcrypto-3-x64.dll" "$bin_dir/"
cp "$path/libssl-3-x64.dll"    "$bin_dir/"
cp "$path/libjpeg-8.dll"       "$bin_dir/"

# remove *.a files
find "$bin_dir/" -name '*.a' -delete

exit 0
