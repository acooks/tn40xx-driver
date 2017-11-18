#!/bin/bash

set -e

# Use the kernel's scripts/Lindent utility
if [ -z "$LINDENT" -o ! -x "$LINDENT" ] ; then
	echo " You need to set the LINDENT environment variable to specify where the Lindent script is (Hint: in the kernel sources at scripts/Lindent)"
	exit 1
fi

# Run Lindent at the start of cleanup, so that the regexes below are applied to
# a predictable style of code.
# Run Lindent twice, because the operation is unfortunately not idempotent
# after only a single run (but is after two runs).
${LINDENT} *.c *.h ; ${LINDENT} *.c *.h

# Trim trailing whitespace
sed -i 's|\s*$||g' *.c *.h


# build 'ccmtcnvt' for converting C++ comments into C comments.
# (unfortunately it doesn't fix C++ comments inside C comments)
make -C cleanup

for f in *.c *.h ; do
	mv $f $f~
	cleanup/ccmtcnvt $f~ > $f
done


# remove horizontal line breaks like /* ------ */
sed -i 's|/\*\s*---*---*\s*\*/||g' *.c


# remove comments after the closing function brace, like } /* foo() */
sed -i 's|\s*}\s*/\*.*\*/|}|g' *.c


# remove the use of the ENTER macro
sed -i 's|\s*ENTER\s*;||g' *.c
# remove the definition of the ENTER macro
sed -i 's|.*#define\s\+ENTER.*||' *.h


# remove the use of the EXIT macro
sed -i 's|\s*EXIT\s*;||g' *.c
# remove the definition of the EXIT macro
sed -i 's|.*#define\s\+EXIT.*||' *.h


# remove the use of the RET() macro
sed -i 's|\(\s\+\)RET(\(.*\));|\1return \2;|' *.c
# remove the definition of the RET() macro
sed -i 's|.*#define\s\+RET\(.*\).*||' *.h


# remove dead code disguised as comments
# weed out function calls first
sed -i 's|/\*\s*.*(.*);.*\*/||g' *.c *.h
# weed out assignments involving struct members
sed -i 's|/\*\s*.*\..*=.*;.*\*/||g' *.c *.h
sed -i 's|/\*\s*.*->.*=.*;.*\*/||g' *.c *.h
sed -i 's|/\*\s*.*=.*->.*;.*\*/||g' *.c *.h


# remove unused g_ftrace variable
sed -i 's|\(.*g_ftrace.*\)||g' *.{c,h}


# remove unused DBG1 macro
sed -i 's|\(#define DBG1.*\)||g' tn40.h
# confirm that it really is unsused and gone
(! grep -Iq DBG1 *.{c,h}) || (echo 'unexpected DBG1 macro remaining.' && false)


# remove redundant return at end of void functions
# look for one level of tab indentation to determine function scope, instead of
# early returns from block scope. It's ugly, but semantically-important
# whitespace works for python and Lindent ensures that the indentation is
# consistent here.
sed -i 's|^\treturn;.*||g' *.c


# Insert new cleanup steps above this comment.
# Keep Lindent as the last step.

# Run Lindent again at the end, because the regexes above affect the format.
# Run Lindent twice, because the operation is unfortunately not idempotent
# after only a single run (but is after two runs).
${LINDENT} *.c *.h ; ${LINDENT} *.c *.h

