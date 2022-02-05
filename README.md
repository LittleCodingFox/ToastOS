# ToastOS
x86_64 OS created from scratch (very incomplete)

# Screenshots

![Basic system usage](https://i.imgur.com/FpEcLRC.png)

![Running some apps](https://i.imgur.com/JtBnvwo.png)

![Doom!](https://i.imgur.com/UJlDDkt.jpg)

# Features

* Scheduling
* Usermode
* tarfs ramdisk
* Hosted toolchain
* mlibc-based usermode libc
* PS/2 keyboard
* Can run Doom
* Coreutils (ls, cat, etc)
* Runs on real hardware (somewhat)

# Requirements

## Linux

Packages:
* xbstrap
* meson
* ninja-build
* clang
* gcc
* g++
* flex
* bison
* gmp
* mpfr
* mpc
* texinfo
* python3-pip
* qemu-system-x86
* help2man
* autopoint
* gperf
* nasm
* llvm

In a debian-based linux environment, the following command should install everything:
`sudo apt install build-essential meson ninja-build clang flex bison libgmp3-dev libmpc-dev libmpfr-dev texinfo python3-pip qemu-system-x86 help2man autopoint gperf nasm llvm && pip3 install xbstrap`

# Building the toolchain

Run `make bootstrap` and wait, it will build the hosted toolchain, ported software, and mlibc.

# Building the OS

Typically you'll want to either run `make clean run-linux` or `make clean debug-linux`.
