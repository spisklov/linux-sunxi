#!/bin/bash

KDIR="../.."
CHECKPATCH="./scripts/checkpatch.pl --strict --file "

pushd $KDIR > /dev/null

FLIST=`find ./drivers/bootscreen -name "*.[ch]"`

if [ -z "$1" ]; then
	for f in $FLIST
	do
		if [ ! -e $f ]; then
			continue
		fi

		$CHECKPATCH --terse $f
	done
else
	$CHECKPATCH drivers/bootscreen/$1
fi

popd > /dev/null
