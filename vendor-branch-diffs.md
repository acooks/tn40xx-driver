# Vendor drops

This is a list of changes between vendor drops.

To generate the diff, use something like this:
```
git diff -D cleanup/v0.3.6.14.1 cleanup/v0.3.6.14.3
```

## vendor-drop/v0.3.6.14.3
- Removed support for Marvell PHYs, which removes support for the following NICs:
 - "TN9210 10GBase-T Ethernet Adapter"
 - "TN9710P 10GBase-T/NBASE-T Ethernet Adapter"
 - "TN9710Q 5GBase-T/NBASE-T Ethernet Adapter"
 - "Edimax 10 Gigabit Ethernet PCI Express Adapter"
 - "Buffalo LGY-PCIE-MG Ethernet Adapter"

Plus the following claims from the vendors release_notes:
- Fix RX Flow Control
- Fix ethtool mtu on Ubuntu 17.04
- Improve single PHY build code
- Fix compilation error for CentOS 7.4
- Fix missing PCP bits in VLAN tag

## vendor-drop/v0.3.6.14.1
- First import; see release_notes for history.
