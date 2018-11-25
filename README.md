
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

# Install

While upstreaming is the ultimate goal of this project, some systems already rely on this driver. For such systems, DKMS provides a convenient way to install and update the driver, [See DKMS instructions](docs/dkms.md).

# Branches

This repo makes extensive use of branches to organise the changes to the vendor driver. For example,
- `release/tn40xx-001`
- `vendor-drop/v0.3.6.17`
- `cleanup/v0.3.6.17`
- `topic/topic/feature/add-Promise-SANLink3-T1`

The branching strategy is [described in the documentation](docs/branches.md).

# Changes between vendor releases

See  [vendor-diffs.md](docs/vendor-diffs.md).

# Thanks

To the following people who have contributed to this project - thank you!
- Jean-Christophe Heger
- Nickolai Zeldovich
