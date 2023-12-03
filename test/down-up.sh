# A simple down-up test

[[ $(type -t now) == function ]] || source $(dirname ${BASH_SOURCE[0]})/include.sh

ip link set dev $IFNAME down && [[ "down" == $(cat /sys/class/net/$IFNAME/operstate) ]] || fail "Failed to set $IFNAME down"
ip link set dev $IFNAME up && (sleep 3; [[ "up" == $(cat /sys/class/net/$IFNAME/operstate) ]]) || fail "Failed to set $IFNAME up"
