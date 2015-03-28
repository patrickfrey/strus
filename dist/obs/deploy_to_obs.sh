#!/bin/sh

PACKAGE_NAME=strus
PACKAGE_VERSION=0.0.1
OSC_HOME=$HOME/home:andreas_baumann/$PACKAGE_NAME

rm -f ${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz
cmake .
make dist-gz

# Redhat

cp ${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz $OSC_HOME/${PACKAGE_NAME}_${PACKAGE_VERSION}.tar.gz
cp dist/redhat/$PACKAGE_NAME.spec $OSC_HOME/$PACKAGE_NAME.spec

# Debian/Ubuntu

SIZE=`stat -c '%s' $OSC_HOME/${PACKAGE_NAME}_${PACKAGE_VERSION}.tar.gz`
CHKSUM=`md5sum $OSC_HOME/${PACKAGE_NAME}_${PACKAGE_VERSION}.tar.gz  | cut -f 1 -d' '`

cat dist/obs/$PACKAGE_NAME.dsc > $OSC_HOME/$PACKAGE_NAME.dsc
echo " $CHKSUM $SIZE ${PACKAGE_NAME}_${PACKAGE_VERSION}.tar.gz" >> $OSC_HOME/$PACKAGE_NAME.dsc

# Archlinux

cp ${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz $OSC_HOME/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz

CHKSUM=`md5sum $OSC_HOME/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz  | cut -f 1 -d' '`

cat dist/archlinux/PKGBUILD > $OSC_HOME/PKGBUILD
echo "md5sums=('$CHKSUM')" >> $OSC_HOME/PKGBUILD
