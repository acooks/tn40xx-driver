# Common functions used by tests.
# Implementation may be provided by test runner instead.

function now() {
        declare -a _uptime
        readarray -d " " -t _uptime < /proc/uptime
        printf "[%12s][USPACE] " "${_uptime[0]}0000"
}
export -f now

function fail() {
	[[ -z $1 ]] || echo "FATAL: $1"
}
export -f fail

function check_dmesg() {
	dmesg | grep -e "BUG" "WARNING" "ERROR" && fail "Kernel log says something is broken."
}
export -f check_dmesg

now "Loading $0"
