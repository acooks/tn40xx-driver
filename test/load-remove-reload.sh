# Load; unload; reload.

[[ $(type -t now) == function ]] || source $(dirname ${BASH_SOURCE[0]})/include.sh

now "Loading"
modprobe tn40xx || fail "Failed to load module tn40xx"
sleep 1;
[[ -d "/sys/bus/pci/drivers/tn40xx" ]] || fail "Failed to register tn40xx driver"
[[ -h "/sys/bus/pci/drivers/tn40xx/${DEV_PCI_ADDRESS}" ]] || fail "Failed to associate tn40xx driver with expected PCIe device"

now "Removing"
modprobe -vr tn40xx || fail "Failed to remove module"

now "Reloading"
modprobe tn40xx || fail "Failed to re-load module tn40xx"
sleep 1;
[[ -d "/sys/bus/pci/drivers/tn40xx" ]] || fail "Failed to re-register tn40xx driver"
[[ -h "/sys/bus/pci/drivers/tn40xx/${DEV_PCI_ADDRESS}" ]] || fail "Failed to re-associate tn40xx driver with expected PCIe device"


