# Branches

This repo makes extensive use of branches to organise the changes to the vendor driver. For example,
- `vendor-drop/v0.3.6.14.3`
- `cleanup/v0.3.6.14.3`
- `topic/remove-linux-2.6-support`

A description of the purpose of each type of branch follows.

## Release branches

A release branch is a combination of (vendor release + cleanups + topic work). The release branch name is a simple monotonically increasing counter of the release number.


## Vendor drop branches

Vendor releases are imported from their distributed tarball into `vendor-drop/` branches. These branches are used for easy access to the unmodified driver, as well as reference between vendor branches.


## Cleanup branches

Ignoring the Linux kernel style prevents this driver from being included in the mainline kernel.

The intention of the `cleanup/` branches is to transform the vendor code into a better format that addresses the problems with the code style used by the vendor.

Some specific problems are:
- mixed tabs and spaces look horrible unless tab-width is 4
- trailing whitespace
- mixed C-style and C++ style comments
- single lines of commented code smells
- long lines


The cleanup transformation MUST:
- be logically equivalent to the vendor code
- follow an automatable, repeatable process
- involve low human effort
- make progress towards an 'upstreamable' driver


To apply the cleanup changes, run: `cleanup.sh`


## Topic branches

Features, bugfixes and improvements towards mainline inclusion are contained on these branches. For example, mainline Linux drivers do not have support for different kernel versions than the one that they ship with, so that support has to be removed.

Topic branches are always based on cleanup branches.

