#!/bin/sh

export BINDIR=bin

rm -f $BINDIR/*.vdi

VBoxManage convertfromraw $BINDIR/disk.img $BINDIR/hdimage-$$.vdi

rm -f $BINDIR/*.bin
