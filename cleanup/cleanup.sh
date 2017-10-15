#!/bin/bash

set -e


# Trim trailing whitespace
sed -i 's|\s*$||g' *.c *.h


# build 'ccmtcnvt' for converting C++ comments into C comments.
# (unfortunately it doesn't fix C++ comments inside C comments)
make -C cleanup

for f in *.c *.h ; do
	mv $f $f~
	cleanup/ccmtcnvt $f~ > $f
done


# Use the kernel's scripts/Lindent utility
if [ -z "$LINDENT" -o ! -x "$LINDENT" ] ; then
	echo " You need to set the LINDENT environment variable to specify where the Lindent script is (Hint: in the kernel sources at scripts/Lindent)"
	exit 1
fi

# Run Lindent twice, because the operation is unfortunately not idempotent
# after only a single run (but is after two runs).
${LINDENT} *.c *.h ; ${LINDENT} *.c *.h

