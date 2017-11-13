
This repo contains the tn40xx Linux driver for 10Gbit NICs based on the TN4010 MAC from [Tehuti Networks](http://www.tehutinetworks.net).

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

An official tn40xx driver is distributed by Tehuti Networks under GPLv2 license, with all the copyright entitlements and restrictions associated with GPLv2 works. OEM vendors (like those listed above) also provide tn40xx drivers from their websites.

This repo does not claim any copyright over the original Tehuti Networks source code. As far as possible, the source code from Tehuti Networks is preserved in unmodified state in the `vendor-drop/` branches.

This repo aims to:
- track the official code drops from the vendor website
- enable comparisons with code drops from OEMs for other NIC implementations that use the same driver
- make the driver easily downloadable for Linux distributions and build systems (with a predictable URL and without web forms and cookies)
- track non-vendor bug fixes and improvements
- track the transformation of the driver into an upstreamable state

The older TN3020-D (Luxor) processor already has a mainline Linux driver, but that driver doesn't support the TN40xx (Bordeaux) devices.

# Branches

This repo makes extensive use of branches to organise the changes to the vendor driver. For example,
- `release/tn40xx-001-linux-4.13`
- `vendor-drop/v0.3.6.14.3`
- `cleanup/v0.3.6.14.3`
- `topic/remove-linux-2.6-support`

A description of the purpose of each type of branch follows.

## Release branches

When a new combination of (vendor release + cleanups + topic work) is ready, a release branch will be made for it. The release branch name indicates the upstream version of Linux it applies to and a simple monotonically increasing counter of the release number.

For example:
- `release/tn40xx-001-linux-4.13` would be the first release of the forked driver and applies to linux-4.13.
- `release/tn40xx-002-linux-4.4` would be the second release of the forked driver and applies to linux-4.4.

Use the largest release number that applies to the kernel version you're building.


## Vendor drop branches

Vendor releases are imported from their distributed tarball into `vendor-drop/` branches. These branches are used for easy access to the unmodified driver, as well as reference between vendor branches.


## Cleanup branches

The intention of the `cleanup/` git branches is to transform the vendor code into a better format that addresses the problems with the code style used by the vendor.

Some specific problems are:
- mixed tabs and spaces look horrible unless tab-width is 4
- trailing whitespace
- mixed C-style and C++ style comments
- single lines of commented code smells
- long lines
- *most importantly, it doesn't match the Linux kernel style, which prevents this driver from ever being merged into the upstream Linux kernel*


The cleanup transformation MUST:
- be logically equivalent to the vendor code
- follow an automatable, repeatable process
- involve low human effort
- make progress towards an 'upstreamable' driver


To apply the cleanup changes, run: `cleanup.sh`


## Topic branches

Features, bugfixes and improvements towards mainline inclusion are contained on these branches. For example, mainline Linux drivers do not have support for different kernel versions than the one that they ship with, so that support has to be removed.

Topic branches are always based on cleanup branches.

# Changes between vendor releases

This is a summary of changes between vendor releases. It is included here, because the `release_notes` file provided by the vendor is not guaranteed to be completely accurate.

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
