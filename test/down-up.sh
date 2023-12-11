#!/usr/bin/bash

# A simple down-up test

[[ $(type -t now) == function ]] ||
	source $(dirname ${BASH_SOURCE[0]})/include.sh

ktap "KTAP version 1"
ktap "1..2"

(ip link set dev $IFNAME down &&
	[[ "down" == $(cat /sys/class/net/$IFNAME/operstate) ]]) &&
	ktap "ok 1 link_set_down" ||
	(
		ktap "# Failed to set $IFNAME down"
		ktap "not ok 1 link_set_down"
	)

(ip link set dev $IFNAME up &&
	(
		sleep 3
		[[ "up" == $(cat /sys/class/net/$IFNAME/operstate) ]]
	)) &&
	ktap "ok 2 link_set_up" ||
	(
		ktap "# Failed to set $IFNAME up"
		ktap "not ok link_set_up"
	)
