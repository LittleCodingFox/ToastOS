#
# mykernel/Makefile
#
# Copyright (C) 2017 - 2020 bzt (bztsrc@gitlab)
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use, copy,
# modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#
# This file is part of the BOOTBOOT Protocol package.
# @brief An example Makefile for sample kernel
#
#

CFLAGS = -Wall -fpic -ffreestanding -fno-stack-protector -nostdinc -nostdlib -Ibootboot -Isrc

SRCDIR   = src
RESDIR   = .
OBJDIR   = obj
BINDIR   = bin
TMPDIR	 = tmp

SOURCES  := $(wildcard $(SRCDIR)/*.c)
RESOURCES  := $(wildcard $(RESDIR)/*.psf)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
RESOURCEOBJECTS := $(RESOURCES:$(RESDIR)/%.psf=$(OBJDIR)/%.o)

all: kernel iso

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	x86_64-elf-gcc $(CFLAGS) -mno-red-zone -c $< -o $@ 

$(RESOURCEOBJECTS): $(OBJDIR)/%.o : $(RESDIR)/%.psf
	x86_64-elf-ld -r -b binary -o $@ $< 

kernel: $(OBJECTS) $(RESOURCEOBJECTS)
	x86_64-elf-ld -nostdlib -nostartfiles -T $(SRCDIR)/link.ld $(OBJECTS) $(RESOURCEOBJECTS) -o $(BINDIR)/mykernel.x86_64.elf
	x86_64-elf-strip -s -K mmio -K fb -K bootboot -K environment $(BINDIR)/mykernel.x86_64.elf
	x86_64-elf-readelf -hls $(BINDIR)/mykernel.x86_64.elf >$(BINDIR)/mykernel.x86_64.txt

iso:
	sh makeiso.sh

clean:
	rm $(OBJDIR)/*.o bin/*.elf bin/*.txt $(BINDIR)/*.img
	rm -Rf tmp

.PHONY: all clean iso
