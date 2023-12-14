# Branches

Historically, this repo made extensive use of branches to organise the changes to the vendor driver. For example,
- `vendor-drop/v0.3.6.14.3`
- `cleanup/v0.3.6.14.3`
- `topic/remove-linux-2.6-support`

A description of the purpose of each type of branch follows.

## Develop branch

Like many git projects, the `develop` branch is the main trunk where most topic branches are based on and merge back to.

## Release branches

The release branch name uses a simple monotonically increasing counter of the release number.

Releases 001 to 004 were based on a combination of (vendor release + cleanups + topic work).

Since there is no more vendor to produce further vendor releases, future releases of this driver can follow the more familiar git workflow where releases branch off `develop`.

## Topic branches

Features, bugfixes and improvements towards mainline inclusion are developed on these branches.

Topic branches used to be based on cleanup branches, and were merged together for a release, but can now follow the usual pattern of branching patterns. Most topic branches will branch from `develop` and merging back to `develop`.


## Vendor drop branches

Vendor releases were imported from their distributed tarball into `vendor-drop/` branches. These branches provide access to the historical unmodified driver, as well as reference between vendor branches.


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



