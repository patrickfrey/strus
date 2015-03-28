#!/bin/sh

PACKAGE_NAME=strus
PACKAGE_VERSION=0.0.1
OSC_HOME=$HOME/home:andreas_baumann/$PACKAGE_NAME

rm -f ${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz
cmake .
make dist-gz

# Redhat/SuSE

cp ${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz $OSC_HOME/${PACKAGE_NAME}_${PACKAGE_VERSION}.tar.gz
cp dist/redhat/$PACKAGE_NAME.spec $OSC_HOME/$PACKAGE_NAME.spec

# Debian/Ubuntu

cp ${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz $OSC_HOME/${PACKAGE_NAME}_${PACKAGE_VERSION}.orig.tar.gz

SIZE=`stat -c '%s' $OSC_HOME/${PACKAGE_NAME}_${PACKAGE_VERSION}.orig.tar.gz`
CHKSUM=`md5sum $OSC_HOME/${PACKAGE_NAME}_${PACKAGE_VERSION}.orig.tar.gz | cut -f 1 -d' '`

cp dist/obs/$PACKAGE_NAME.dsc $OSC_HOME/$PACKAGE_NAME.dsc
echo " $CHKSUM $SIZE ${PACKAGE_NAME}_${PACKAGE_VERSION}.orig.tar.gz" >> $OSC_HOME/$PACKAGE_NAME.dsc

TMPDIR=/tmp
rm -f $OSC_HOME/${PACKAGE_NAME}_${PACKAGE_VERSION}.debian.tar.gz
rm -rf $TMPDIR/debian
cp -r dist/debian $TMPDIR/.
OLDDIR=$PWD
cd $TMPDIR
tar zcf $TMPDIR/${PACKAGE_NAME}_${PACKAGE_VERSION}.debian.tar.gz debian
cd $OLDDIR
mv -f $TMPDIR/${PACKAGE_NAME}_${PACKAGE_VERSION}.debian.tar.gz $OSC_HOME/.
DEBIAN_SIZE=`stat -c '%s' $OSC_HOME/${PACKAGE_NAME}_${PACKAGE_VERSION}.debian.tar.gz`
DEBIAN_CHKSUM=`md5sum  $OSC_HOME/${PACKAGE_NAME}_${PACKAGE_VERSION}.debian.tar.gz | cut -f 1 -d' '`
echo " $DEBIAN_CHKSUM $DEBIAN_SIZE ${PACKAGE_NAME}_${PACKAGE_VERSION}.debian.tar.gz" >> $OSC_HOME/$PACKAGE_NAME.dsc

# Archlinux

cp ${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz $OSC_HOME/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz

CHKSUM=`md5sum $OSC_HOME/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz  | cut -f 1 -d' '`

cat dist/archlinux/PKGBUILD > $OSC_HOME/PKGBUILD
echo "md5sums=('$CHKSUM')" >> $OSC_HOME/PKGBUILD
