all: boot.S bootboot.c fs.h
	@echo "  src		aarch64-rpi (Raspberry Pi 3+)"
	@aarch64-elf-gcc -Wall -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles -fno-stack-protector -c boot.S -o boot.o
	@aarch64-elf-gcc -Wall -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles -fno-stack-protector -I ./ -c tinflate.c -o tinflate.o
	@aarch64-elf-gcc -Wall -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles -fno-stack-protector -I ./ -c bootboot.c -o bootboot.o
	@aarch64-elf-ld -r -b binary -o font.o font.psf
	@aarch64-elf-ld -nostdlib -nostartfiles boot.o bootboot.o tinflate.o font.o -T link.ld -o bootboot.elf
	@aarch64-elf-objcopy -O binary bootboot.elf ../bootboot.img
	@rm *.o bootboot.elf

mkboot: mkboot.c
	gcc mkboot.c -o mkboot

raspbootcom: raspbootcom.c
	gcc raspbootcom.c -o raspbootcom

getfw:
	wget -q https://raw.githubusercontent.com/raspberrypi/firmware/master/boot/LICENCE.broadcom
	wget -q https://raw.githubusercontent.com/raspberrypi/firmware/master/boot/bootcode.bin
	wget -q https://raw.githubusercontent.com/raspberrypi/firmware/master/boot/fixup.dat
	wget -q https://raw.githubusercontent.com/raspberrypi/firmware/master/boot/start.elf

cleanfw:
	@rm LICENCE.broadcom bootcode.bin fixup.dat start.elf

clean:
	@rm mkboot raspbootcom *.o >/dev/null 2>/dev/null || true
