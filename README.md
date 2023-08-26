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
* SMP (multiple cores)
* Hosted toolchain
* mlibc-based usermode libc
* PS/2 keyboard and mouse
* Input API
* Graphics API (basic)
* Mesa port (OpenGL ES1/2/3)
* Coreutils (ls, cat, etc)
* Can run Doom
* Runs on real hardware (somewhat)

# Requirements

## Linux

### Debian
`sudo apt install build-essential gcc gettext make perl libssl-dev ninja-build clang flex bison libgmp3-dev libmpc-dev libmpfr-dev texinfo python3-pip qemu-system-x86 help2man autopoint gperf nasm llvm mercurial groff && pip3 install xbstrap && pip3 install mako && pip3 install meson`

### Fedora

`sudo dnf install g++ binutils patch gcc gettext make perl openssl-devel ninja-build clang flex bison gmp-devel libmpc-devel mpfr-devel texinfo python3-pip qemu-system-x86 help2man gperf nasm llvm mercurial groff gettext-devel && pip3 install xbstrap && pip3 install mako && pip3 install meson`

### Arch

`sudo pacman -S gcc gettext make perl openssl ninja clang flex bison gmp mpc mpfr texinfo python-pip qemu-system-x86 help2man gperf nasm llvm mercurial groff python-mako meson`

# Building the toolchain

Run `make bootstrap` and wait, it will build the hosted toolchain, ported software, and mlibc.

# Building the OS

Typically you'll want to either run `make clean run-linux` or `make clean debug-linux`. You can optionally add `KVM=1` at the end for faster emulation if your system supports KVM, and `SMP=X` where `X` is the amount of virtual CPUs you want it to use.
