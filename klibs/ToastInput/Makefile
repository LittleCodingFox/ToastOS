export PATH := $(PATH):../../toolchain/tools/host-binutils/bin:../../toolchain/tools/host-gcc/bin

CC				= x86_64-toast-g++
LD				= x86_64-toast-ld

PROGRAM_NAME	= ToastInput
SRCDIR			= src
OBJDIR			= obj/$(PROGRAM_NAME)

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

CSRC			= $(call rwildcard,$(SRCDIR),*.c)

COBJECTS		= $(CSRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

INCLUDEDIRS		= -I../../toolchain/system-root/usr/include -I../../userland/include

CFLAGS			= $(INCLUDEDIRS) -fshort-wchar -Wall -O0 -g -fno-omit-frame-pointer \
	-fno-rtti -fno-exceptions -fno-stack-protector -mno-3dnow -mno-mmx -mno-avx -fPIC

default: $(PROGRAM_NAME)

makedirs:
	mkdir -p $(OBJDIR)
	mkdir -p $(DESTDIR)/usr/include/toast/
	mkdir -p $(DESTDIR)/usr/lib

$(PROGRAM_NAME): makedirs $(COBJECTS)
	$(LD) -shared -o "$(DESTDIR)/usr/lib/lib$@.so" $(COBJECTS)
	cp -Rf ../../userland/include/* $(DESTDIR)/usr/include/

$(COBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) -c $< -o $@
