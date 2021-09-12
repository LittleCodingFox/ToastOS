CC = clang

PROGRAM_NAME	= test
BINDIR 			= ../../dist/bin
SRCDIR			= src
OBJDIR			= ../obj/userland/test

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

CSRC			= $(call rwildcard,$(SRCDIR),*.c)

COBJECTS		= $(CSRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

INCLUDEDIRS		= -I../../klibc

CFLAGS			= $(INCLUDEDIRS) -ffreestanding -fshort-wchar -Wall -O0 -fno-omit-frame-pointer \
	-fno-rtti -fno-exceptions -fno-builtin -fno-pie -fno-stack-protector -fno-rtti -fno-exceptions -mno-3dnow \
	-mno-mmx -mno-sse -mno-sse2 -mno-avx

LDFLAGS			= -Ttext-segment=40000000 -nostdlib -no-pie -L../../bin/libc -lc -Map=linker.map 

default: test

makedirs:
	mkdir -p ../obj/userland/test

test: makedirs $(COBJECTS)
	mkdir -p $(BINDIR)
	$(LD) $(COBJECTS) $(LDFLAGS) -o $(BINDIR)/$(PROGRAM_NAME)
	awk '$$1 ~ /0x[0-9a-f]{16}/ {print substr($$1, 3), $$2}' linker.map > symbols.map
	rm linker.map

$(COBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) -c $< -o $@
