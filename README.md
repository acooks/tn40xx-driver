
This repo contains the tn40xx Linux driver for 10Gbit NICs based on the TN4010 MAC from [Tehuti Networks](http://www.tehutinetworks.net).

An official tn40xx driver is distributed by Tehuti Networks under GPLv2 license, with all the copyright entitlements and restrictions associated with GPLv2 works. OEM vendors also provide tn40xx driver from their websites.

This repo does not claim any copyright over the original Tehuti Networks source code. As far as possible, the source code from Tehuti Networks is preserved in unmodified state in the `vendor-drop/` branches.

This repo aims to:
- track the official code drops from the vendor website
- enable comparisons with code drops from OEMs for other NIC implementations that use the same driver
- make the driver easily downloadable for Linux distributions and build systems (with a predictable URL and without web forms and cookies)
- track non-vendor bug fixes and improvements
- track the transformation of the driver into an upstreamable state


See cleanup-branches.md and vendor-branch-diffs.md for more information about changes to the vendor code.
