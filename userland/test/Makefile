CC = clang

PROGRAM_NAME	= test
BINDIR 			= ../../dist/bin
SRCDIR			= src
OBJDIR			= ../obj/userland/test

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

CSRC			= $(call rwildcard,$(SRCDIR),*.c)

COBJECTS		= $(CSRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

INCLUDEDIRS		= -I../../src/klibc

CFLAGS			= $(INCLUDEDIRS) -ffreestanding -fshort-wchar -Wall -O0 -fno-omit-frame-pointer \
	-fno-rtti -fno-exceptions -fno-builtin -fno-pie

LD_FLAGS = -Wl, -Ttext-segment=40000000 -nostdlib -no-pie

default: test

makedirs:
	mkdir -p ../obj/userland/test

test: makedirs $(COBJECTS)
	mkdir -p $(BINDIR)
	$(LD) $(LDFLAGS) $(COBJECTS) -o $(BINDIR)/$(PROGRAM_NAME)

$(COBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) -c $< -o $@
