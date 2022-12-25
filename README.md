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
`sudo apt install build-essential gcc gettext make perl libssl-dev ninja-build clang flex bison libgmp3-dev libmpc-dev libmpfr-dev texinfo python3-pip qemu-system-x86 help2man autopoint gperf nasm llvm mercurial groff libexpat1-dev zlib1g-dev x11-apps libxml2-dev itstool && pip3 install xbstrap && pip3 install mako && pip3 install meson && pip3 install libxml2-python3`

### Fedora

`sudo dnf install g++ binutils patch gcc gettext make perl openssl-devel ninja-build clang flex bison gmp-devel libmpc-devel mpfr-devel texinfo python3-pip qemu-system-x86 help2man gperf nasm llvm mercurial groff gettext-devel expat-devel zlib-devel xcursorgen libxml2-devel itstool && pip3 install xbstrap && pip3 install mako && pip3 install meson && pip3 install libxml2-python3`

# Building the toolchain

Run `make bootstrap` and wait, it will build the hosted toolchain, ported software, and mlibc.

# Building the OS

Typically you'll want to either run `make clean run-linux` or `make clean debug-linux`. You can optionally add `ENABLE_KVM=1` at the end to have a faster emulation if your system supports KVM.
