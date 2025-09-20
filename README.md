# YmirC

YmirC is an AMD-V/SVM based type-1 hypervisor, written in C. It is designed to run Linux as a guest OS (tested with v6.15.9).

![YmirC demo](assets/demo.gif)

This project is based on the blog series [Writing Hypervisor in Zig](https://hv.smallkirby.com/).

## Requirements

- GNU-EFI — required for building the hypervisor.
- QEMU — for running the hypervisor.
- OVMF — UEFI firmware for QEMU.
- AMD x86_64 CPU — Intel CPUs are not supported.

Installation Example (Ubuntu/Debian)

```
sudo apt install gcc make qemu-system-x86 ovmf gnu-efi
```

Adjust the package manager commands if you are using a different Linux distribution.

## Run

First, install the sample guest kernel and initramfs:

```sh
make install-linux
```

Then, launch the hypervisor inside QEMU:

```sh
make run
```
