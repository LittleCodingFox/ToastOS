CC				= x86_64-elf-gcc
LD				= x86_64-elf-ld
STRIP			= x86_64-elf-strip
READELF			= x86_64-elf-readelf
CFLAGS			= -Wall -fpic -ffreestanding -fno-stack-protector -nostdinc -nostdlib -Ibootboot/dist -Isrc -Isrc/include -O0
KERNEL_NAME		= kernel

SRCDIR  		= src
RESDIR   		= .
OBJDIR   		= obj
BINDIR   		= bin
TMPDIR	 		= tmp

SOURCES         := $(wildcard $(SRCDIR)/*.c)
SOURCES         += $(wildcard $(SRCDIR)/**/*.c)
SOURCES         += $(wildcard $(SRCDIR)/low-level/**/*.c)
RESOURCES 		:= $(wildcard $(RESDIR)/*.psf)
OBJECTS 		:= $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
RESOURCEOBJECTS := $(RESOURCES:$(RESDIR)/%.psf=$(OBJDIR)/%.o)

makedirs:
	rm -Rf bin
	rm -Rf tmp
	mkdir -p bin
	mkdir -p obj

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) -mno-red-zone -c $< -o $@ 

$(RESOURCEOBJECTS): $(OBJDIR)/%.o : $(RESDIR)/%.psf
	mkdir -p $(shell dirname $@)
	$(LD) -r -b binary -o $@ $< 

kernel: makedirs $(OBJECTS) $(RESOURCEOBJECTS)
	$(LD) -nostdlib -nostartfiles -T $(SRCDIR)/link.ld $(OBJECTS) $(RESOURCEOBJECTS) -o $(BINDIR)/$(KERNEL_NAME).x86_64.elf
	$(STRIP) -s -K mmio -K fb -K bootboot -K environment $(BINDIR)/$(KERNEL_NAME).x86_64.elf
	$(READELF) -hls $(BINDIR)/$(KERNEL_NAME).x86_64.elf >$(BINDIR)/$(KERNEL_NAME).x86_64.txt

bootdir:
	sh makebootdir.sh
	
iso-osx: bootdir
	sh makeisoosx.sh
	
iso-linux: bootdir
	sh makeisolinux.sh

run-linux: kernel iso-linux
	qemu-system-x86_64 -bios /usr/share/qemu/OVMF.fd -drive file=$(BINDIR)/disk.img,format=raw,index=0,media=disk -serial file:./debug.log -d int --no-reboot

clean:
	rm -Rf $(BINDIR)/*.img
	rm -Rf $(BINDIR)/*.iso
	rm -Rf $(BINDIR)/*.cdr
	rm -Rf $(BINDIR)/*.bin
	rm -Rf $(BINDIR)/*.vdi
	rm -f bin/*.elf bin/*.txt $(BINDIR)/*.img
	rm -Rf tmp
	rm -Rf boot
	rm -Rf obj

.PHONY: all clean bootdir iso-osx iso-linux makedirs
