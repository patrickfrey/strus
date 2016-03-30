#!/bin/sh

# DEBIAN
export DEBEMAIL="patrickpfrey@yahoo.com"
export DEBFULLNAME="Patrick Frey"
PACKAGEID="strus-0.0"

rm -Rf pkg/$PACKAGEID
mkdir pkg/$PACKAGEID
cp -Rf include/ pkg/$PACKAGEID/include
mkdir -p pkg/$PACKAGEID/lib/strus
find src -name "*.so" -exec cp {} pkg/$PACKAGEID/lib/strus/ \;

cd pkg/$PACKAGEID
dh_make --email patrickpfrey@yahoo.com --copyright=MPL2 --createorig -i
rm debian/README.Debian
rm debian/init.d.ex 
rm debian/manpage.1.ex 
rm debian/manpage.sgml.ex 
rm debian/manpage.xml.ex 
rm debian/menu.ex 
rm debian/postinst.ex
rm debian/postrm.ex
rm debian/preinst.ex
rm debian/prerm.ex 
rm debian/strus.default.ex
rm debian/strus.doc-base.EX 
rm debian/strus.cron.d.ex 

