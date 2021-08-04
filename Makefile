CC				= clang
LD				= ld
STRIP			= x86_64-elf-strip
READELF			= x86_64-elf-readelf
GNUEFI			= gnu-efi
OVMFDIR			= OVMFbin
ASMC			= nasm
BOOTEFI 		:= $(GNUEFI)/x86_64/bootloader/main.efi

KERNEL_NAME		= kernel

OS_NAME			= ToastOS

SRCDIR  		= src
OBJDIR   		= obj
BINDIR   		= bin
TMPDIR	 		= tmp

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

SRC				= $(call rwildcard,$(SRCDIR),*.cpp)
CSRC			= $(call rwildcard,$(SRCDIR),*.c)
EXTSRC			= $(call rwildcard, $(SRCDIR)/../ext-libs/vtconsole,*.cpp)
EXTCSRC			= $(call rwildcard, $(SRCDIR)/../ext-libs/printf,*.c)
EXTCSRC			+= $(call rwildcard, $(SRCDIR)/../ext-libs/liballoc,*.c)
ASMSRC			= $(call rwildcard,$(SRCDIR),*.asm)
OBJECTS			= $(SRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
ASMOBJECTS		= $(ASMSRC:$(SRCDIR)/%.asm=$(OBJDIR)/%_asm.o)
COBJECTS		= $(CSRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
EXTOBJECTS		= $(EXTSRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
EXTCOBJECTS		= $(EXTCSRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

INCLUDEDIRS		= -Isrc -Isrc/klibc -Isrc/include -Isrc/low-level -Iext-libs -Iext-libs/liballoc/
ASMFLAGS		=
CFLAGS			= $(INCLUDEDIRS) -ffreestanding -fshort-wchar -nostdlib -mno-red-zone -Wall -fpic -O3 -fno-omit-frame-pointer \
	-fstack-protector-all -fno-rtti -fno-exceptions
LDFLAGS			= -T $(SRCDIR)/link.ld -static -Bsymbolic -nostdlib -Map=linker.map

QEMU_FLAGS		=

makedirs:
	mkdir -p bin
	mkdir -p obj

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

kernel: makedirs $(OBJECTS) $(COBJECTS) $(EXTOBJECTS) $(EXTCOBJECTS) $(ASMOBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) $(COBJECTS) $(EXTOBJECTS) $(EXTCOBJECTS) $(ASMOBJECTS) -o $(BINDIR)/kernel.elf
	awk '$$1 ~ /0x[0-9a-f]{16}/ {print substr($$1, 3), $$2}' linker.map > symbols.map
	rm linker.map

iso-linux:
	sh makebootdir.sh
	sh makeisolinux.sh

run-linux: gnuefi kernel iso-linux
	qemu-system-x86_64 -drive file=$(BINDIR)/$(OS_NAME).img,format=raw,index=0,media=disk \
	-bios /usr/share/qemu/OVMF.fd \
	-m 256M -cpu qemu64 -machine type=q35 -serial file:./debug.log -net none -d int --no-reboot $(QEMU_FLAGS)

debug-linux: CFLAGS += -DKERNEL_DEBUG=1 -g
#debug: QEMU_FLAGS += -s -S
debug-linux: run-linux

gnuefi:
	(cd gnu-efi && make && make bootloader)

clean:
	(cd gnu-efi && make clean)
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

.PHONY: all clean iso run makedirs debug
