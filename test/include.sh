#!/usr/bin/bash

# Common functions used by tests; not meant to be executed by itself.
# Implementation may be provided by test runner instead.

function now() {
	declare -a _uptime
	readarray -d " " -t _uptime < /proc/uptime
	printf "[%12s][USPACE] " "${_uptime[0]}0000 "
	echo "$@"
}
export -f now

function fail() {
	[[ -z $1 ]] || echo "FATAL: $1"
}
export -f fail

# Define a specific name for generating KTAP output, to distinguish its
# intentional use from general debugging noise.
function ktap() {
	echo "$@"
}
export -f ktap

# A generic catch-all check for issues appearing in the kernel log.
# May be overridden by specific tests to avoid spurious failures.
function check_dmesg() {
	if (dmesg | grep -E 'BUG|WARNING|ERROR'); then
		ktap "# Kernel log says something is broken."
		return 1
	fi
	return 0
}
export -f check_dmesg
