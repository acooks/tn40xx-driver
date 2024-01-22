
This repo contains the tn40xx Linux driver for 10Gbit NICs based on the TN4010 MAC from the (now defunct) Tehuti Networks.

This driver enables the following 10Gb SFP+ NICs:
- D-Link DXE-810S
- Edimax EN-9320SFP+
- StarTech PEX10000SFP
- Synology E10G15-F1

... as well as the following 10GBase-T/NBASE-T NICs:
- D-Link DXE-810T
- Edimax EN-9320TX-E
- EXSYS EX-6061-2
- Intellinet 507950
- StarTech ST10GSPEXNB

This repo aims to:
- slowly transform the driver into a potentially upstreamable state
- make it easily downloadable for Linux distributions and build systems

The older TN3020-D (Luxor) processor has a mainline Linux driver, but that driver doesn't support the TN40xx (Bordeaux) devices.

# Problematic NICs

If you have a NIC that combines the tn40xx MAC with a Marvell PHY, you will have to go on a quest to obtain the firmware for the PHY. It cannot be distributed or supported by this driver, because Marvell has not permitted redistribution of the firmware.

## How to tell if you have an unsopported PHY
   ```
   lspci -vnn
   ```

will output something like this:

   ```
    0a:00.0 Ethernet controller [0200]: Tehuti Networks Ltd. TN9210 10GBase-T Ethernet Adapter [1fc9:4024]
	Subsystem: Tehuti Networks Ltd. Device [1fc9:3015]
   ```

   In the example above, 0x1fc9 is the Vendor ID and 0x4024 is the Device ID. 0x3015 is the Subsystem Device ID.

   The problematic devices are identified by these combinations of Device ID and Subsystem Device ID:
   - 4024, 3015 "TN9210 10GBase-T Ethernet Adapter",
   - 4027, 3015 "TN9710P 10GBase-T/NBASE-T Ethernet Adapter",
   - 4027, 8104 "Edimax 10 Gigabit Ethernet PCI Express Adapter",
   - 4027, 0368 "Buffalo LGY-PCIE-MG Ethernet Adapter",
   - 4027, 1546 "IOI GE10-PCIE4XG202P 10Gbase-T/NBASE-T Ethernet Adapter",
   - 4027, 1001 "LR-Link LREC6860BT 10 Gigabit Ethernet Adapter",
   - 4027, 3310 "QNAP PCIe Expansion Card",
   - 4527, 3015 "TN9710Q 5GBase-T/NBASE-T Ethernet Adapter"


# Install

While upstreaming is the ultimate goal of this project, some systems already rely on this driver. For such systems, DKMS provides a convenient way to install and update the driver, [See DKMS instructions](docs/dkms.md).

# Branches

This repo contains several long-lived branches. You may have to look around for support for the kernel version and PHY that you have.
- `release/linux-6.6.y-1` -> Linux > 6.6.x
- `release/tn40xx-006` -> Linux > 5.15.x
- `release/tn40xx-004` -> Linux > 5.4
- `release/tn40xx-003` -> Linux <= 5.4
- `vendor-drop/v0.3.6.17`

More detail on branching in [described in the documentation](docs/branches.md).

# Thanks

To the following people who have contributed to this project - thank you!
- Jean-Christophe Heger
- Nickolai Zeldovich
- Patrick Heffernan
- Carsten Heinz

# Origin

A tn40xx driver was distributed by Tehuti Networks under GPLv2 license, as well as various OEM vendors, like those listed above.

The last release of the driver by Tehuti Networks was version 0.3.6.17.2, imported to this repo in commit 894992c904a6b0e99 on Jan 7 2020.

On Jan 9 2020 a message was posted to Tehuti Networks' LinkedIn page, announcing that it has ceased operations.

The original source releases have been preserved in unmodified state in the `vendor-drop/` branches.

This repo does not claim any copyright over the original Tehuti Networks source code, but it has been significantly modified and continues to evolve and hopefully improve.
