export PATH := $(PATH):../../toolchain/tools/host-binutils/bin:../../toolchain/tools/host-gcc/bin

CC				= x86_64-toast-g++
LD				= x86_64-toast-ld

PROGRAM_NAME	= dwm
BINDIR 			= ../../dist/bin
SRCDIR			= src
OBJDIR			= ../obj/userland/$(PROGRAM_NAME)

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

CSRC			= $(call rwildcard,$(SRCDIR),*.cpp)

COBJECTS		= $(CSRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

INCLUDEDIRS		= -I../../toolchain/system-root/usr/include -I../include -I../../frigg/include -I../../klibc

CFLAGS			= $(INCLUDEDIRS) -fshort-wchar -Wall -O0 -g -fno-omit-frame-pointer \
	-fno-exceptions -fno-stack-protector -mno-3dnow \
	-mno-mmx -mno-avx

LDFLAGS			= -L../../toolchain/pkg-builds/mlibc/ \
				  -L../../toolchain/packages/mlibc/usr/lib \
				  -L../../dist/usr/lib \
				  -L../../toolchain/system-root/usr/lib \
				  -L../../toolchain/tools/host-gcc/x86_64-toast/lib64 \
				  -lToastGraphics -lToastInput -lGLESv1_CM -lOSMesa -lstdc++ -lc -lgcc_s -Map=linker.map

default: $(PROGRAM_NAME)

makedirs:
	mkdir -p ../obj/userland/$(PROGRAM_NAME)

$(PROGRAM_NAME): makedirs $(COBJECTS)
	mkdir -p $(BINDIR)
	$(LD) $(COBJECTS) $(LDFLAGS) ../../toolchain/packages/mlibc/usr/lib/crt0.o -o $(BINDIR)/$(PROGRAM_NAME)
	awk '$$1 ~ /0x[0-9a-f]{16}/ {print substr($$1, 3), $$2}' linker.map > symbols.map
	rm linker.map

$(COBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) -c $< -o $@
