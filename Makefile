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

OVMF				= /usr/share/qemu/OVMF.fd

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

SRC					= $(call rwildcard,$(SRCDIR),*.cpp)
CSRC				= $(call rwildcard,$(SRCDIR),*.c)
EXTSRC				= $(call rwildcard, $(SRCDIR)/../ext-libs/vtconsole,*.cpp)
EXTCSRC				= $(call rwildcard, $(SRCDIR)/../ext-libs/printf,*.c)
EXTCSRC				+= $(call rwildcard, $(SRCDIR)/../ext-libs/liballoc,*.c)
EXTCSRC				+= $(call rwildcard, $(SRCDIR)/../ext-libs/osdev-paging-x64,*.c)
EXTCSRC				+= $(call rwildcard, $(SRCDIR)/../lai/core,*.c)
EXTCSRC				+= $(call rwildcard, $(SRCDIR)/../lai/drivers,*.c)
EXTCSRC				+= $(call rwildcard, $(SRCDIR)/../lai/helpers,*.c)
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

KASANEXCLUSIONS		= $(wildcard $(SRCDIR)/*.cpp)
KASANEXCLUSIONS 	+= $(wildcard $(SRCDIR)/drivers/AHCI/*.cpp)
KASANEXCLUSIONS 	+= $(wildcard $(SRCDIR)/low-level/gdt/*.cpp)
KASANEXCLUSIONS 	+= $(wildcard $(SRCDIR)/low-level/interrupts/*.cpp)
KASANEXCLUSIONS 	+= $(wildcard $(SRCDIR)/low-level/kasan/*.cpp)
KASANEXCLUSIONS 	+= $(wildcard $(SRCDIR)/low-level/paging/*.cpp)
KASANEXCLUSIONS 	+= $(wildcard $(SRCDIR)/low-level/ports/*.cpp)
KASANEXCLUSIONS 	+= $(wildcard $(SRCDIR)/low-level/process/fd/*.cpp)
KASANEXCLUSIONS 	+= $(wildcard $(SRCDIR)/low-level/registers/*.cpp)
KASANEXCLUSIONS 	+= $(wildcard $(SRCDIR)/low-level/serial/*.cpp)
KASANEXCLUSIONS 	+= $(wildcard $(SRCDIR)/low-level/sse/*.cpp)
KASANEXCLUSIONS 	+= $(wildcard $(SRCDIR)/low-level/stacktrace/*.cpp)
KASANEXCLUSIONS 	+= $(wildcard $(SRCDIR)/low-level/syscall/*.cpp)
KASANEXCLUSIONS 	+= $(wildcard $(SRCDIR)/low-level/threading/*.cpp)
KASANEXCLUSIONS 	+= $(wildcard $(SRCDIR)/low-level/*.cpp)

KASANOBJECTS 		= $(KASANEXCLUSIONS:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

INCLUDEDIRS			= 	-Isrc \
						-Iklibc \
						-Isrc/include \
						-Isrc/low-level \
						-Iext-libs \
						-Iext-libs/liballoc/ \
						-Ifrigg/include \
						-Icxxshim/stage2/include \
						-Ilai/include

ASMFLAGS			= -g -F dwarf
ASFLAGS 			= -nostdlib -fpic
CFLAGS				= $(INCLUDEDIRS) -ffreestanding -fshort-wchar -nostdlib -mno-red-zone -Wall -fpic -O3 -fno-omit-frame-pointer -g \
	-fno-stack-protector -fno-rtti -fno-exceptions -mno-3dnow -mno-mmx -mno-sse -mno-sse2 -mno-avx -fno-builtin \
	-Werror -Wno-ambiguous-reversed-operator -Wno-c99-designator -Wno-deprecated-volatile -Wno-initializer-overrides \
	-Wno-unused-private-field -Wno-ignored-attributes\
	-DPRINTF_DISABLE_SUPPORT_FLOAT=1 -DPRINTF_DISABLE_SUPPORT_EXPONENTIAL=1 --target=x86_64-pc-none-elf -march=x86-64
CFLAGS_INTERNAL		= 
LDFLAGS				= -T $(SRCDIR)/link.ld -static -Bsymbolic -nostdlib -Map=linker.map -zmax-page-size=0x1000
QEMU_FLAGS			= 
QEMU_EXTRA_FLAGS 	= 
CFLAGS_KASAN		= -fsanitize=kernel-address -mllvm -asan-mapping-offset=0xdfffe00000000000 -mllvm -asan-globals=false

SMP					?= 2

ifeq ($(UBSAN), 1)
	CFLAGS_INTERNAL += -fsanitize=undefined -fno-sanitize=function
endif

ifeq ($(KVM), 1)
	QEMU_EXTRA_FLAGS += -enable-kvm
endif

ifeq ($(KASAN), 1)
	CFLAGS += -DUSE_KASAN=1
endif

makedirs:
	mkdir -p bin
	mkdir -p $(LIBCBINDIR)
	mkdir -p obj
	mkdir -p $(LIBCOBJDIR)
	mkdir -p dist/bin
	mkdir -p dist/tmp

$(COBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	mkdir -p $(shell dirname $@)
	@echo compile C $@
ifeq ($(KASAN), 1)
	$(CC) $(CFLAGS) $(CFLAGS_KASAN) $(CFLAGS_INTERNAL) -c $< -o $@
else
	$(CC) $(CFLAGS) $(CFLAGS_INTERNAL) -c $< -o $@
endif

$(EXTCOBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	mkdir -p $(shell dirname $@)
	@echo compile ext C $@
	$(CC) $(CFLAGS) -c $< -o $@

$(EXTOBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	mkdir -p $(shell dirname $@)
	@echo compile ext cpp $@
	$(CPP) $(CFLAGS) -c $< -o $@

ifeq ($(KASAN), 1)

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	mkdir -p $(shell dirname $@)
	@echo compile kasan cpp $@
	$(CPP) $(CFLAGS) $(CFLAGS_INTERNAL) $(CFLAGS_KASAN) -std=c++20 -c $< -o $@

else

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	mkdir -p $(shell dirname $@)
	@echo compile cpp $@
	$(CPP) $(CFLAGS) $(CFLAGS_INTERNAL) -std=c++20 -c $< -o $@

endif

$(KASANOBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	mkdir -p $(shell dirname $@)
	@echo compile kasan exclusion cpp $@
	$(CPP) $(CFLAGS) $(CFLAGS_INTERNAL) -std=c++20 -c $< -o $@

$(ASMOBJECTS): $(OBJDIR)/%_asm.o : $(SRCDIR)/%.asm
	mkdir -p $(shell dirname $@)
	@echo compile asm $@
	$(ASMC) $(ASMFLAGS) $< -f elf64 -o $@

$(ASOBJECTS): $(OBJDIR)/%_asm.o : $(SRCDIR)/%.s
	mkdir -p $(shell dirname $@)
	@echo compile gas $@
	$(ASC) $(ASFLAGS) -c $< -o $@

$(LIBCCOBJECTS): CFLAGS += -DIS_LIBC
$(LIBCCOBJECTS): $(LIBCOBJDIR)/%.o : $(LIBCSRCDIR)/%.c
	mkdir -p $(shell dirname $@)
	@echo compile libc C $@
	$(CC) $(CFLAGS) $(CFLAGS_INTERNAL) -c $< -o $@

$(LIBKCOBJECTS): CFLAGS += -DIS_LIBK
$(LIBKCOBJECTS): $(LIBKOBJDIR)/%.o : $(LIBCSRCDIR)/%.c
	mkdir -p $(shell dirname $@)
	@echo compile libk C $@
	$(CC) $(CFLAGS) $(CFLAGS_INTERNAL) -c $< -o $@

$(LIBCASMOBJECTS): $(LIBCOBJDIR)/%_asm.o : $(LIBCSRCDIR)/%.asm
	mkdir -p $(shell dirname $@)
	@echo compile libc asm $@
	$(ASMC) $(ASMFLAGS) $< -f elf64 -o $@

kernel: makedirs $(KASANOBJECTS) $(KASANCOBJECTS) $(OBJECTS) $(COBJECTS) $(LIBKCOBJECTS) $(EXTOBJECTS) $(EXTCOBJECTS) $(ASMOBJECTS) $(ASOBJECTS)
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
debug-linux: run-linux

debug-gdb-linux: QEMU_FLAGS = -s -S
debug-gdb-linux: debug-linux

run-qemu-linux:
	qemu-system-x86_64 -drive file=$(BINDIR)/$(OS_NAME).img,format=raw,index=0,media=disk \
	-bios $(OVMF) -display gtk $(QEMU_FLAGS) $(QEMU_EXTRA_FLAGS) -vga std -smp $(SMP) \
	-m 4G -cpu qemu64 -machine type=q35 -serial file:./debug.log -netdev user,id=u1 -device rtl8139,netdev=u1 \
	-d int --no-reboot 2>qemu.log

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
