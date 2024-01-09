introduction
============
Dynamic Kernel Module Support (DKMS) builds Linux kernel modules whose sources reside outside the kernel source tree. It automates rebuilding of such modules when a new kernel is installed.

install
=======
    git clone -b release/linux-6.6.y-1 https://github.com/acooks/tn40xx-driver.git /usr/src/tn40xx-linux-6.6.y-1
    dkms add -m tn40xx -v linux-6.6.y-1

build driver for current kernel
===============================
    dkms install -m tn40xx -v linux-6.6.y-1

build driver for specific kernel version
========================================
    dkms install -m tn40xx -v linux-6.6.y-1 -k [kernel_version]

uninstall
=========
This will remove module for all kernel versions

    dkms remove -m tn40xx -v linux-6.6.y-1 --all
