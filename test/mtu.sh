# Iterate over a list of MTU sizes

[[ $(type -t now) == function ]] || source $(dirname ${BASH_SOURCE[0]})/include.sh

# Lets try a few MTU sizes
declare -a MTUS=(604 1179 1180 1499 1500 1520 1650 8900 9000 9100 16384)
now "Walking MTU sizes: ${MTUS[@]}"

for MTU in ${MTUS[@]}; do
        ip link set dev $IFNAME mtu $MTU || fail "Failed to set MTU $MTU"
        (ip --oneline link show dev $IFNAME | grep "mtu $MTU") || fail "Failed to find expected MTU $MTU"
        sleep 1;
done
now "MTU walk done"
