
# Changes between vendor releases

The vendor provides [release_notes](release_notes), Sometimes additional information comes to light when comparing the cleaned-up branches - that's where these review notes may help.

To generate the diff, use something like this:
```
git diff -D cleanup/v0.3.6.15 cleanup/v0.3.6.16.1
```

## vendor-drop/v0.3.6.17
- AQR105 PHY firmware blob updated from v2.b.e-798 to v2.c.7-880
- Add QNAP's TN9710P SVID/SDID (uses MV88X3310 PHY)
- Add LR-Link LREC6860BT SVID/SDID (uses MV88X3310 PHY)
- Add LR-Link LREC6860AF SVID/SDID (uses TLK10232 PHY)
- QNAP LED support to MV88X3310 PHY driver
- Bump expected MV88E2010 and MV88X3310 firmware blob version from `0_3_3_0_9374` to `0_3_4_0_9445`
- PCI IRQ MSI initialisation changed
- Add device shutdown callback

## vendor-drop/v0.3.6.16-startech
- Startech OEM driver contains Marvell firmware files missing from vendor-drop/v0.3.6.16.

## vendor-drop/v0.3.6.16
- AQR105 PHY firmware updated from XXX to 2.b.e2
- Add SVID/SDID for [IOI GE10-PCIE4XG202](http://www.ioi.com.tw/products/prodcat_device.aspx?CatID=106&DeviceID=3035&HostID=2069) OEM products. They're using _Marvell MV88X3310 PHYs_, so Linux support may be a challenge.

## vendor-drop/v0.3.6.15
- The Marvell PHY control that was removed in 0.3.6.14.3 has reappeared (without the required firmware/initialisation data blobs!) and the 'Marvell' name has been replaced with 'Pele' for what could only be trademark lawyer nonsense.
- Added support for "Mustang-200 10GbE Ethernet Adapter"
- More magical _Mike fix_ copy pasta
- Some timer cruft was removed

Plus vendor's release_notes:
- New build method for Marvell PHYs. Please consult the Readme file
- Improved startup procudure for "Phyless" CX4 mode
- Dynamic advertising of MACA for RX flow control
- Support for IEI SVID/SDID added
- Improved memory allocation for memory limited or fragmented applications

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
