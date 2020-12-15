#!/bin/sh

cd $(dirname $0)

OPK_NAME=PocketSNES.opk

echo Building ${OPK_NAME}...

# create opk
FLIST="../pocketsnes/PocketSNES.dge default.gcw0.desktop ../pocketsnes/backdrop.png sfc.png manual-cn.txt myfont.ttf"

rm -f ${OPK_NAME}
mksquashfs ${FLIST} ${OPK_NAME} -all-root -no-xattrs -noappend -no-exports

cat default.gcw0.desktop
