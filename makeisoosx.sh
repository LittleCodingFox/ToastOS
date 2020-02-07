#!/bin/sh

export BINDIR=bin

hdiutil create -fs MS-DOS -layout NONE -sectors 2880 -srcfolder ./boot -format UDTO -o $BINDIR/disk

#IMAGE="$(echo $BINDIR/disk.cdr | sed -E 's/(\..{0,3})?\.cdr$/.img/')"
#mv "$BINDIR/disk.cdr" $IMAGE

#sh makevboxdisk.sh

echo Done!
