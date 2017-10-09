# Vendor code drop cleanups

The intention of the `cleanup/` git branches is to transform the vendor code into a better format that addresses the problems with the code style used by the vendor.

Some specific problems are:
- mixed tabs and spaces look horrible unless tab-width is 4
- trailing whitespace
- mixed C-style and C++ style comments
- single lines of commented code smells
- long lines
- *most importantly, it doesn't match the Linux kernel style, which prevents this driver from ever being merged into the upstream Linux kernel*


The transformation must:
- be logically equivalent to the vendor code
- follow an automatable, repeatable process
- involve low human effort
- make progress towards an 'upstreamable' driver


To apply the cleanup changes, run: `cleanup.sh`
