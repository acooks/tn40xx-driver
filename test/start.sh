#!/usr/bin/bash

# These tests are likely disruptive to the system running the driver,
# as well as the systems on the same network.
#
# Run these tests in a VM with PCI passthrough.
# See https://github.com/rapido-linux/rapido

pushd "$(dirname ${BASH_SOURCE[0]})"

source include.sh

DEV_PCI_ADDRESS=$1
MODULE=$2

ktap "KTAP version 1"
ktap "1..5"

ktap "# Loading $MODULE module"
./load-unload-reload.sh $MODULE $DEV_PCI_ADDRESS &&
	ktap "ok 1 load_unload_reload" ||
	ktap "not ok 1 load_unload_reload"

check_dmesg &&
	ktap "ok 2 check_dmesg_after_load_unload_reload" ||
	ktap "not ok 2 check_dmesg_after_load_unload_reload"

IFNAME=$(basename /sys/devices/pci0000\:00/${DEV_PCI_ADDRESS}/net/*)
export IFNAME

ktap "# Testing operstate up->down->up setting"
./down-up.sh &&
	ktap "ok 3 up_down_up" ||
	ktap "not ok 3 up_down_up"

check_dmesg &&
	ktap "ok 4 check_dmesg_after_up_down_up" ||
	ktap "not ok 4 check_dmesg_after_up_down_up"

ktap "# Testing MTU changing"
./mtu.sh &&
	ktap "ok 5 change_mtu" ||
	ktap "not ok 5 change_mtu"

#now "Waiting for dynamic IP addresses..."
#tn40xx-test/get-dhcp-address.sh

./pktgen.sh

popd
ktap "# tn40xx tests complete :)"
