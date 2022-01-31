CC					= clang
CPP					= clang++
LD					= ld
AR					= llvm-ar
READELF				= llvm-readelf
ASMC				= nasm
ASC					= clang

KERNEL_NAME			= kernel

OS_NAME				= ToastOS

SRCDIR  			= src
OBJDIR   			= obj/kernel
BINDIR   			= bin
TMPDIR	 			= tmp
DISTDIR				= dist

LIBCSRCDIR 			= klibc

LIBCBINDIR			= bin/libc
LIBCOBJDIR			= obj/libc
LIBKOBJDIR			= obj/libk

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

SRC					= $(call rwildcard,$(SRCDIR),*.cpp)
CSRC				= $(call rwildcard,$(SRCDIR),*.c)
EXTSRC				= $(call rwildcard, $(SRCDIR)/../ext-libs/vtconsole,*.cpp)
EXTCSRC				= $(call rwildcard, $(SRCDIR)/../ext-libs/printf,*.c)
EXTCSRC				+= $(call rwildcard, $(SRCDIR)/../ext-libs/liballoc,*.c)
LIBCSRC				= $(call rwildcard,$(LIBCSRCDIR),*.c)
ASMSRC				= $(call rwildcard,$(SRCDIR),*.asm)
ASSRC				= $(call rwildcard,$(SRCDIR),*.s)
LIBCASMSRC			= $(call rwildcard,$(LIBCSRCDIR),*.asm)
OBJECTS				= $(SRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
ASMOBJECTS			= $(ASMSRC:$(SRCDIR)/%.asm=$(OBJDIR)/%_asm.o)
ASOBJECTS			= $(ASSRC:$(SRCDIR)/%.s=$(OBJDIR)/%_asm.o)
COBJECTS			= $(CSRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
EXTOBJECTS			= $(EXTSRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
EXTCOBJECTS			= $(EXTCSRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
LIBCCOBJECTS		= $(LIBCSRC:$(LIBCSRCDIR)/%.c=$(LIBCOBJDIR)/%.o)
LIBKCOBJECTS		= $(LIBCSRC:$(LIBCSRCDIR)/%.c=$(LIBKOBJDIR)/%.o)
LIBCASMOBJECTS		= $(LIBCASMSRC:$(LIBCSRCDIR)/%.asm=$(LIBCOBJDIR)/%_asm.o)

INCLUDEDIRS			= -Isrc -Iklibc -Isrc/include -Isrc/low-level -Iext-libs -Iext-libs/liballoc/ -Ifrigg/include -Icxxshim/stage2/include
ASMFLAGS			= -g -F dwarf
ASFLAGS 			=  -nostdlib -fpic
CFLAGS				= $(INCLUDEDIRS) -ffreestanding -fshort-wchar -nostdlib -mno-red-zone -Wall -fpic -O3 -fno-omit-frame-pointer -g \
	-fno-stack-protector -fno-rtti -fno-exceptions -mno-3dnow -mno-mmx -mno-sse -mno-sse2 -mno-avx \
	-Werror -Wno-ambiguous-reversed-operator -Wno-c99-designator -Wno-deprecated-volatile -Wno-initializer-overrides
CFLAGS_INTERNAL		= 
LDFLAGS				= -T $(SRCDIR)/link.ld -static -Bsymbolic -nostdlib -Map=linker.map -zmax-page-size=0x1000
QEMU_FLAGS			= -enable-kvm
QEMU_EXTRA_FLAGS 	= 

ifeq ($(USE_UBSAN), 1)
	CFLAGS_INTERNAL += -fsanitize=undefined -fno-sanitize=function
endif

ifeq ($(ENABLE_KVM), 1)
	QEMU_EXTRA_FLAGS = -enable-kvm
endif

makedirs:
	mkdir -p bin
	mkdir -p $(LIBCBINDIR)
	mkdir -p obj
	mkdir -p $(LIBCOBJDIR)
	mkdir -p dist/bin

$(COBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) $(CFLAGS_INTERNAL) -c $< -o $@

$(EXTCOBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(EXTOBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	mkdir -p $(shell dirname $@)
	$(CPP) $(CFLAGS) -c $< -o $@

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	mkdir -p $(shell dirname $@)
	$(CPP) $(CFLAGS) $(CFLAGS_INTERNAL) -std=c++20 -c $< -o $@

$(ASMOBJECTS): $(OBJDIR)/%_asm.o : $(SRCDIR)/%.asm
	mkdir -p $(shell dirname $@)
	$(ASMC) $(ASMFLAGS) $< -f elf64 -o $@

$(ASOBJECTS): $(OBJDIR)/%_asm.o : $(SRCDIR)/%.s
	mkdir -p $(shell dirname $@)
	$(ASC) $(ASFLAGS) -c $< -o $@

$(LIBCCOBJECTS): CFLAGS += -DIS_LIBC
$(LIBCCOBJECTS): $(LIBCOBJDIR)/%.o : $(LIBCSRCDIR)/%.c
	mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) $(CFLAGS_INTERNAL) -c $< -o $@

$(LIBKCOBJECTS): CFLAGS += -DIS_LIBK
$(LIBKCOBJECTS): $(LIBKOBJDIR)/%.o : $(LIBCSRCDIR)/%.c
	mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) $(CFLAGS_INTERNAL) -c $< -o $@

$(LIBCASMOBJECTS): $(LIBCOBJDIR)/%_asm.o : $(LIBCSRCDIR)/%.asm
	mkdir -p $(shell dirname $@)
	$(ASMC) $(ASMFLAGS) $< -f elf64 -o $@

kernel: makedirs $(OBJECTS) $(COBJECTS) $(LIBKCOBJECTS) $(EXTOBJECTS) $(EXTCOBJECTS) $(ASMOBJECTS) $(ASOBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) $(COBJECTS) $(LIBKCOBJECTS) $(EXTOBJECTS) $(EXTCOBJECTS) $(ASMOBJECTS) $(ASOBJECTS) -o $(BINDIR)/kernel
	awk '$$1 ~ /0x[0-9a-f]{16}/ {print substr($$1, 3), $$2}' linker.map > symbols.map
	rm linker.map

libc: makedirs $(LIBCCOBJECTS) $(LIBCASMOBJECTS) $(EXTCOBJECTS)
	$(AR) rcs "$(LIBCBINDIR)/$@.a" $(LIBCCOBJECTS) $(LIBCASMOBJECTS) $(EXTCOBJECTS)
	mkdir -p $(DISTDIR)/lib/x86_64-toast/
	mkdir -p $(DISTDIR)/usr/lib64
	cp toolchain/pkg-builds/mlibc/*.so $(DISTDIR)/lib/x86_64-toast
	cp toolchain/pkg-builds/mlibc/*.so $(DISTDIR)/lib
	cp $(DISTDIR)/lib/ld.so $(DISTDIR)/lib/ld64.so.1
	cp -Rf toolchain/tools/host-gcc/x86_64-toast/lib64/* $(DISTDIR)/usr/lib64/

iso-linux:
	sh makebootdir.sh
	sh makeisolinux.sh

userland: libc
	@for path in $(shell find userland/* -maxdepth 0 -type d); do \
		$(MAKE) -C $$path; \
	done

linux: kernel userland iso-linux

run-linux: linux run-qemu-linux

debug-linux: CFLAGS += -DKERNEL_DEBUG=1 #-O0
debug-linux: QEMU_FLAGS = -s -S
debug-linux: run-linux

run-qemu-linux:
	qemu-system-x86_64 -drive file=$(BINDIR)/$(OS_NAME).img,format=raw,index=0,media=disk \
	-bios /usr/share/qemu/OVMF.fd -display gtk $(QEMU_FLAGS) $(QEMU_EXTRA_FLAGS) \
	-m 1G -cpu qemu64 -machine type=q35 -serial file:./debug.log -net none -d int --no-reboot 2>qemu.log

debug-qemu-linux: QEMU_FLAGS = -s -S
debug-qemu-linux: run-qemu-linux

bootstrap:
	@mkdir -p toolchain
	@mkdir -p ports
	cd toolchain && xbstrap init ..
	cd toolchain && xbstrap install-tool --all
	cd toolchain && xbstrap install mlibc
	cd toolchain && xbstrap install -u --all

rebuild-bootstrap: clean-bootstrap bootstrap

clean-bootstrap:
	rm -Rf toolchain
	rm -Rf ports

rebuild-mlibc:
	cd toolchain && xbstrap build mlibc-headers --reconfigure
	cd toolchain && xbstrap build mlibc --reconfigure
	cd toolchain && xbstrap install mlibc-headers
	cd toolchain && xbstrap install mlibc

clean:
	rm -Rf $(BINDIR)/*.img
	rm -Rf $(BINDIR)/*.iso
	rm -Rf $(BINDIR)/*.cdr
	rm -Rf $(BINDIR)/*.bin
	rm -Rf $(BINDIR)/*.vdi
	rm -f *.map
	rm -f $(BINDIR)/*.img
	rm -Rf bin
	rm -Rf tmp
	rm -Rf boot
	rm -Rf obj
	rm -Rf userland/obj

.PHONY: all clean run-linux makedirs debug-linux userland frigg bootstrap clean-bootstreap
