#!/usr/bin/bash

source $(dirname ${BASH_SOURCE[0]})/include.sh

DEV_PCI_ADDRESS=$1
MODULE=$2

now "Loading Driver"
#tn40xx-test/load-remove-reload.sh
#check_dmesg
modprobe tn40xx


IFNAME=$(basename /sys/devices/pci0000\:00/${DEV_PCI_ADDRESS}/net/*)


now "Testing operstate up->down->up setting"
#tn40xx-test/down-up.sh
#check_dmesg
ip link set dev $IFNAME up

#now; tn40xx-test/mtu.sh

#now "Starting udevd"
#/usr/lib/systemd/systemd-udevd &

#now "Waiting for dynamic IP addresses..."
#tn40xx-test/get-dhcp-address.sh

#now; tn40xx-test/pktgen.sh

now "tn40xx tests complete :)"
