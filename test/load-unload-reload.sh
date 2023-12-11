#!/usr/bin/bash

# Load; unload; reload the driver specified as the first parameter.

[[ $(type -t now) == function ]] ||
	source "$(dirname ${BASH_SOURCE[0]})"/include.sh

MODULE=$1
DEV_PCI_ADDRESS=$2

function _check() {
	if [[ ! -d "/sys/bus/pci/drivers/$MODULE" ]]; then
		ktap "# Failed to register $MODULE driver"
		return 1
	fi

	if [[ ! -L "/sys/bus/pci/drivers/tn40xx/${DEV_PCI_ADDRESS}" ]]; then
		ktap "# Failed to associate $MODULE driver with PCIe device ${DEV_PCI_ADDRESS}"
		return 1
	fi

	return 0
}

ktap "KTAP version 1"
ktap "1..5"

ktap "# loading"
modprobe $MODULE && ktap "ok 1 load" || (
	ktap "# Failed to load module $MODULE"
	ktap "not ok 1 load"
)

sleep 1
_check && ktap "ok 2 check_sysfs" || ktap "not ok 2 check_sysfs"

ktap "# unloading"
modprobe -vr $MODULE && ktap "ok 3 unloading" || (
	ktap "# Failed to remove module"
	ktap "not ok 3 unloading"
)

ktap "# reloading"
modprobe $MODULE && ktap "ok 4 reloading" || (
	ktap "# Failed to re-load module $MODULE"
	ktap "not ok 4 reloading"
)

sleep 1
_check && ktap "ok 5 check_sysfs" || ktap "not ok 5 check_sysfs"
