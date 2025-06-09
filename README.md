# YmirC

YmirC is an AMD-V based type-1 hypervisor, written in C.

This project is being developed based on the blog series [Writing Hypervisor in Zig](https://hv.smallkirby.com/).

**YmirC is currently under development.**

## Requirements

- GNU-EFI (to build)
- QEMU (to run)
- OVMF (to run)
## Run

```sh
$ make run
```

This will launch the hypervisor inside QEMU.  
**Note:** The host CPU must be an AMD x86_64 processor — Intel CPUs are not supported.
