export PATH := $(PATH):../../toolchain/tools/host-binutils/bin:../../toolchain/tools/host-gcc/bin

CC				= x86_64-toast-g++
AR				= x86_64-toast-ar

PROGRAM_NAME	= toastinput
BINDIR 			= ../../dist/usr/lib
SRCDIR			= src
OBJDIR			= ../obj/userland/$(PROGRAM_NAME)

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

CSRC			= $(call rwildcard,$(SRCDIR),*.c)

COBJECTS		= $(CSRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

INCLUDEDIRS		= -I../../toolchain/system-root/usr/include -I../include

CFLAGS			= $(INCLUDEDIRS) -fshort-wchar -Wall -O0 -g -fno-omit-frame-pointer \
	-fno-rtti -fno-exceptions -fno-stack-protector -mno-3dnow \
	-mno-mmx -mno-sse -mno-sse2 -mno-avx -fPIC

default: $(PROGRAM_NAME)

makedirs:
	mkdir -p ../obj/userland/$(PROGRAM_NAME)

$(PROGRAM_NAME): makedirs $(COBJECTS)
	mkdir -p $(BINDIR)
	$(AR) rcs "$(BINDIR)/lib$@.a" $(COBJECTS)

$(COBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) -c $< -o $@
