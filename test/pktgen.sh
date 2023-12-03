# Generate some traffic using pktgen.

[[ $(type -t now) == function ]] || source $(dirname ${BASH_SOURCE[0]})/include.sh

now "Configuring pktgen..."

PKTGEN=/proc/net/pktgen
PKTGEN0=$PKTGEN/kpktgend_0
ETH1_0="${PKTGEN}/eth1@0"

echo add_device eth1@0 > $PKTGEN0
#cat $PKTGEN0
#cat $ETH1_0

echo "count 100000" > $ETH1_0
echo "burst 10" > $ETH1_0
echo "min_pkt_size 1" > $ETH1_0
echo "max_pkt_size 10000" > $ETH1_0
echo "dst_mac 00:00:00:00:12:34" > $ETH1_0
#echo "src_mac 00:00:00:00:12:34" > $ETH1_0 # leave commented to use the NIC's actual MAC
echo "udp_dst_min 1" > $ETH1_0
echo "udp_dst_max 10000" > $ETH1_0

#echo "flag IPSRC_RND" > $ETH1_0 # this would upset the lan
echo "src_min 10.1.4.99" > $ETH1_0
echo "src_max 10.1.4.99" > $ETH1_0

#echo "flag IPDST_RND" > $ETH1_0 # this would upset the lan
echo "dst_min 10.1.4.153" > $ETH1_0
echo "dst_max 10.1.4.153" > $ETH1_0

echo "flag UDPSRC_RND" > $ETH1_0
#echo "flag UDPDST_RND" > $ETH1_0

cat $ETH1_0

echo -e "pktgen will now be started with:\n\techo start > $PKTGEN/pgctrl"
echo "Proceed ? [Y/n]"
read && [[ -z "$REPLY" || "y" == $REPLY ]] && echo start > $PKTGEN/pgctrl

