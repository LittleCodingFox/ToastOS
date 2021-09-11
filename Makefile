CC				= clang
LD				= ld
AR				= x86_64-elf-ar
STRIP			= x86_64-elf-strip
READELF			= x86_64-elf-readelf
OVMFDIR			= OVMFbin
ASMC			= nasm

KERNEL_NAME		= kernel

OS_NAME			= ToastOS

SRCDIR  		= src
OBJDIR   		= obj/kernel
BINDIR   		= bin
TMPDIR	 		= tmp

LIBCSRCDIR 		= klibc

LIBCBINDIR		= bin/libc
LIBCOBJDIR		= obj/libc
LIBKOBJDIR		= obj/libk

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

SRC				= $(call rwildcard,$(SRCDIR),*.cpp)
CSRC			= $(call rwildcard,$(SRCDIR),*.c)
EXTSRC			= $(call rwildcard, $(SRCDIR)/../ext-libs/vtconsole,*.cpp)
EXTCSRC			= $(call rwildcard, $(SRCDIR)/../ext-libs/printf,*.c)
EXTCSRC			+= $(call rwildcard, $(SRCDIR)/../ext-libs/liballoc,*.c)
LIBCSRC			= $(call rwildcard,$(LIBCSRCDIR),*.c)
ASMSRC			= $(call rwildcard,$(SRCDIR),*.asm)
OBJECTS			= $(SRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
ASMOBJECTS		= $(ASMSRC:$(SRCDIR)/%.asm=$(OBJDIR)/%_asm.o)
COBJECTS		= $(CSRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
EXTOBJECTS		= $(EXTSRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
EXTCOBJECTS		= $(EXTCSRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
LIBCCOBJECTS	= $(LIBCSRC:$(LIBCSRCDIR)/%.c=$(LIBCOBJDIR)/%.o)
LIBKCOBJECTS	= $(LIBCSRC:$(LIBCSRCDIR)/%.c=$(LIBKOBJDIR)/%.o)

INCLUDEDIRS		= -Isrc -Iklibc -Isrc/include -Isrc/low-level -Iext-libs -Iext-libs/liballoc/
ASMFLAGS		=
CFLAGS			= $(INCLUDEDIRS) -ffreestanding -fshort-wchar -nostdlib -mno-red-zone -Wall -fpic -O3 -fno-omit-frame-pointer \
	-fno-stack-protector -fno-rtti -fno-exceptions -mno-3dnow -mno-mmx -mno-sse -mno-sse2 -mno-avx
LDFLAGS			= -T $(SRCDIR)/link.ld -static -Bsymbolic -nostdlib -Map=linker.map -zmax-page-size=0x1000

QEMU_FLAGS		=

makedirs:
	mkdir -p bin
	mkdir -p $(LIBCBINDIR)
	mkdir -p obj
	mkdir -p $(LIBCOBJDIR)
	mkdir -p dist/bin
	mkdir -p dist/system

$(COBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(EXTCOBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(EXTOBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(ASMOBJECTS): $(OBJDIR)/%_asm.o : $(SRCDIR)/%.asm
	mkdir -p $(shell dirname $@)
	$(ASMC) $(ASMFLAGS) $< -f elf64 -o $@

$(LIBCCOBJECTS): CFLAGS += -DIS_LIBC
$(LIBCCOBJECTS): $(LIBCOBJDIR)/%.o : $(LIBCSRCDIR)/%.c
	mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIBKCOBJECTS): CFLAGS += -DIS_LIBK
$(LIBKCOBJECTS): $(LIBKOBJDIR)/%.o : $(LIBCSRCDIR)/%.c
	mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) -c $< -o $@

kernel: makedirs $(OBJECTS) $(COBJECTS) $(LIBKCOBJECTS) $(EXTOBJECTS) $(EXTCOBJECTS) $(ASMOBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) $(COBJECTS) $(LIBKCOBJECTS) $(EXTOBJECTS) $(EXTCOBJECTS) $(ASMOBJECTS) -o $(BINDIR)/kernel
	awk '$$1 ~ /0x[0-9a-f]{16}/ {print substr($$1, 3), $$2}' linker.map > symbols.map
	rm linker.map

libc: makedirs $(LIBCCOBJECTS)
	$(AR) rcs "$(LIBCBINDIR)/$@.a" $(LIBCCOBJECTS)

iso-linux:
	sh makebootdir.sh
	sh makeisolinux.sh

userland: libc
	@for path in $(shell find userland/* -type d -maxdepth 0); do \
		$(MAKE) -C $$path; \
	done

run-linux: kernel userland iso-linux run-qemu-linux

debug-linux: CFLAGS += -DKERNEL_DEBUG=1 #-g -O0
#debug: QEMU_FLAGS += -s -S
debug-linux: run-linux

run-qemu-linux:
	qemu-system-x86_64 -drive file=$(BINDIR)/$(OS_NAME).img,format=raw,index=0,media=disk \
	-bios /usr/share/qemu/OVMF.fd \
	-m 256M -cpu qemu64 -machine type=q35 -serial file:./debug.log -net none -d int --no-reboot $(QEMU_FLAGS) 2>qemu.log

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

.PHONY: all clean run-linux makedirs debug-linux userland
