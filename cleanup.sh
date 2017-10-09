#!/bin/bash

set -e

if [ -z "$LINDENT" -o ! -x "$LINDENT" ] ; then
	echo " You need to set the LINDENT environment variable to specify where the Lindent script is (Hint: in the kernel sources at scripts/Lindent)"
	exit 1
fi


if ! which ccmtcnvt >/dev/null ; then
        echo "You need 'ccmtcnvt' in the path. It is provided by the liwc package. Your distro may have it, or get it from https://github.com/ajkaijanaho/liwc"
        exit 1
fi


# Trim trailing whitespace:
sed -i 's|\s*$||g' *.c *.h


# Convert C++ comments into C comments.
# Unfortunately this doesn't fix C++ comments inside C comments
for f in *.c *.h
	do mv $f $f~
	ccmtcnvt $f~ > $f
done


# Run a twice, because the operation is unfortunately not idempotent
# after only a single run (but is after two runs).
${LINDENT} *.c *.h ; ${LINDENT} *.c *.h

