
This is a set of scripts for testing driver functionality.

These scripts are intended to be used with
[rapido](https://github.com/acooks/rapido/tree/tn40xx-testing)
to test the driver in a VM.

Some advantages to this test approach include:
 * the host is protected from memory corruption errors caused by buggy kernel
   drivers;
 * the PCI peripheral can be physically installed in a multi-use machine,
   reducing hardware & lab requirements;
 * debugging info is easily available;
 * the development cycle is short and simple - rapid even :)

PCI Passthrough is required to be able to assign the real hardware peripheral
to the VM.

These tests implement the
[Kernel Test Anything Protocol (KTAP)](https://docs.kernel.org/dev-tools/ktap.html)
to format test results
