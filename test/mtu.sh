#!/usr/bin/bash

# Iterate over a list of MTU sizes

[[ $(type -t now) == function ]] ||
	source $(dirname ${BASH_SOURCE[0]})/include.sh

# Lets try a few MTU sizes
declare -a MTUS=(604 1179 1180 1499 1500 1520 1650 8900 9000 9100 16384)

ktap "KTAP version 1"
ktap "1..${#MTUS[@]}"

ktap "# Walking MTU sizes: ${MTUS[@]}"

let N=1
for MTU in ${MTUS[@]}; do
	ktap "# $MTU"
	ip link set dev $IFNAME mtu $MTU || fail "Failed to set MTU $MTU"
	(ip --oneline link show dev $IFNAME | grep "mtu $MTU") &&
		ktap "ok $N MTU_$MTU" ||
		ktap "not ok $N MTU_$MTU"
	let N++
	sleep 1
done
ktap "# MTU walk done"
