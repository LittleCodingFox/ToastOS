# ToastOS
x86_64 OS created from scratch

# Features

* Scheduling
* Usermode
* tarfs ramdisk
* Hosted toolchain
* mlibc-based usermode libc

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

In a debian-based linux environment, the following command should install everything:
`sudo apt install build-essential meson ninja-build clang flex bison libgmp3-dev libmpc-dev libmpfr-dev texinfo python3-pip qemu-system-x86 && pip3 install xbstrap`

# Building the toolchain

Run `make bootstrap` and wait, it will build the hosted toolchain, ported software, and mlibc.

# Building the OS

Typically you'll want to either run `make clean run-linux` or `make clean debug-linux`.
