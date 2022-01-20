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

In a debian-based linux environment, the following command should install everything:
`sudo apt install build-essential meson ninja-build clang flex bison libgmp3-dev libmpc-dev libmpfr-dev texinfo`

For xbstrap, run the following command:
`pip3 install xbstrap` 
