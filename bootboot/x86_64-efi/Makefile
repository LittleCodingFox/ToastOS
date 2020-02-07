ARCH               = $(shell uname -m | sed s,i[3456789]86,ia32,)
GNUEFI_INCLUDES    = -I/usr/include -I. -I/usr/include/efi -I/usr/include/efi/$(ARCH) -I/usr/include/efi/protocol
GNUEFI_CRT_OBJS    = crt0-efi-$(ARCH).o
GNUEFI_LDS         = elf_$(ARCH)_efi.lds

CFLAGS = -mno-red-zone -mno-mmx -mno-sse -O2 -fpic -Wall -Wextra -Werror -fshort-wchar -fno-strict-aliasing -ffreestanding -fno-stack-protector -fno-stack-check -DCONFIG_$(ARCH) -DGNU_EFI_USE_MS_ABI -maccumulate-outgoing-args --std=c11 

LDFLAGS = -nostdlib
LDFLAGS += -shared -Bsymbolic -L. $(GNUEFI_CRT_OBJS)

TARGET  = bootboot.efi

all: tinflate.o $(TARGET)

%.efi: %.so
	@echo "  src		x86_64-efi (UEFI)"
	@objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-$(ARCH) --subsystem=10 $^ $@
	@echo -n "BOOTBOOT Loader do not " | dd conv=notrunc of=$(TARGET) bs=1 seek=78 1>/dev/null 2>/dev/null
	@gcc $(GNUEFI_INCLUDES) -Wall -fshort-wchar efirom.c -o efirom $(LIBS)
	@./efirom $(TARGET) ../bootboot.rom || true
	@mv $(TARGET) ../$(TARGET)
	@rm tinflate.o efirom

%.so: %.o
	@ld $(LDFLAGS) tinflate.o $^ -o $@ -lefi -lgnuefi -T $(GNUEFI_LDS)

%.o: %.c
	@gcc $(GNUEFI_INCLUDES) $(CFLAGS) -c $< -o $@

%.o: %.S
	@gcc $(GNUEFI_INCLUDES) $(CFLAGS) -c $< -o $@

clean:
	@rm bootboot.o $(TARGET) ../$(TARGET) ../bootboot.rom *.so *.efi efirom tinflate.o 2>/dev/null || true

