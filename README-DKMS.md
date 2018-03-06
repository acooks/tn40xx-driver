introduction
============
Fork of TN40XX driver from https://github.com/acooks/tn40xx-driver, with DKMS configuration file

install
=======
    git clone -b cleanup/v0.3.6.15 https://github.com/jcheger/tn40xx-driver.git /usr/src/tn40xx-0.3.6.15
    dkms add -m tn40xx -v 0.3.6.15

build driver for current kernel
===============================
    dkms install -m tn40xx -v 0.3.6.15
    
build driver for specific kernel version
========================================
    dkms install -m tn40xx -v 0.3.6.15 -k [kernel_version]

uninstall
=========
This will remove module for all kernel versions

    dkms remove -m tn40xx -v 0.3.6.15 --all
