
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


# Problematic Kernel versions

Occasionally people ask for help with old kernels, or kernels based on Enterprise Linux distributions. Unfortunately, this is something that is difficult for me (Andrew) to help with. I haven't got the time, and I believe that Enterprise Linux vendors should be providing this support, and be paid for it. Please seek help from the vendor of the Enterprise Linux distribution, or consider whether your needs and interests may be better aligned with an up-to-date community-supported distribution.

# Install

While upstreaming is the ultimate goal of this project, some systems already rely on this driver. For such systems, DKMS provides a convenient way to install and update the driver, [See DKMS instructions](docs/dkms.md).

# Branches

This repo contains several long-lived branches. You may have to look around for support for the kernel version and PHY that you have.
- `release/linux-6.7.y-1` --> 6.7.x <= Linux
- `release/linux-6.6.y-1` --> 6.6.x <= Linux < 6.8
- `release/tn40xx-006`    --> 5.15.x <= Linux < 6.8
- `release/tn40xx-004`    --> 5.4 < Linux < 5.15
- `release/tn40xx-003`    --> Linux <= 5.4
- `vendor-drop/v0.3.6.17`

More detail on branching in [described in the documentation](docs/branches.md).

## Example Driver Install (for a Tehuti Networks TN9710P)
1.  Confirm what your Device ID using this command `lspci -nn | grep Ethernet`.  You'll see two numbers in brackets at the end of the line, e.g. `[1fc9:4027]`.  The Device ID is the second number.

2.  Determine your kernel version with `uname -a`.
3.  Make sure you have your kernel headers installed.  This varies from system to system, but on Ubuntu the command is something along the lines of `apt install linux-headers-...`.
4.  Determine which branch you need (see the next section).  For example, with Linux 6.5.13, you would use `release/tn40xx-006`.
5.  Clone this repo into `/usr/src/tn40xx` with the following command: `git clone -b [CORRECT BRANCH] https://github.com/acooks/tn40xx-driver.git /usr/src/tn40xx`
6.  If you have a problematic device, such as the TN9710P (Device ID 4027), you'll need to manually find and install the HDR file for the firmware.  More discussion as to why this is [here](https://github.com/acooks/tn40xx-driver/issues/3).  There may be helpful links to aid in your quest for the file there.
7.  Once you have your HDR file (for example x3310fw_0_3_4_0_9445.hdr), save it to your repo in `/usr/src/tn40xx`
8.  Check to make sure you have `xxd` installed with command `xxd -v`.  If it's not present, install it (e.g. `apt install xxd`).
9.  Run the `mvidtoh.sh` (found in the root directory of the repo) with this command `bash mvidtoh.sh [Name of HDR file] [Model] [.h file]`.  For example, `bash mvidtoh.sh x3310fw_0_3_4_0_9445.hdr MV88X3310 MV88X3310_phy.h`
10.  From here, you can run `make` and `make install`
11.  Load the module with `insmod tn40xx`
12.  To see if the module loaded correctly, run `dmesg`.  You should have a message that says something along the lines of "PHY detected on port...".  If you don't see a message like that or it says something along the lines of "PHY failed", then it's very likely you didn't build the correct driver.  Try to figure out which is the right driver for you and repeat.

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
