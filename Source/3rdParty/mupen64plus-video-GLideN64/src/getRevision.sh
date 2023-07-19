#!/bin/sh

set -eu

cd -- "$(cd -- "${0%/*}/" && pwd -P)"

header="${OUTPUT:-./Revision.h}"

if [ "${1:-}" != --nogit ]
then
   rev="\"$(git rev-parse --short HEAD)\"" || rev='""'
else
   rev='""'
fi

lastrev=''
if [ -e "$header" ]
then
   lastrev="$(head -n 1 "$header" 2> /dev/null | cut -d ' ' -f3)"
fi

printf '%s\n' "current revision $rev" "last build revision $lastrev"

if [ "$lastrev" != "$rev" ]
then
  [ "$header" = "${header##*/}" ] || mkdir -p -- "${header%/*}"
   printf '%s\n' "#define PLUGIN_REVISION $rev" \
      "#define PLUGIN_REVISION_W L$rev" > "$header"
fi
