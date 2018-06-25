introduction
============
Dynamic Kernel Module Support (DKMS) builds Linux kernel modules whose sources reside outside the kernel source tree. It automates rebuilding of such modules when a new kernel is installed.

install
=======
    git clone -b release/tn40xx-001 https://github.com/acooks/tn40xx-driver.git /usr/src/tn40xx-001
    dkms add -m tn40xx -v 001

build driver for current kernel
===============================
    dkms install -m tn40xx -v 001

build driver for specific kernel version
========================================
    dkms install -m tn40xx -v 001 -k [kernel_version]

uninstall
=========
This will remove module for all kernel versions

    dkms remove -m tn40xx -v 001 --all
