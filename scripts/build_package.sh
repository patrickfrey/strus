#!/bin/sh

# DEBIAN
PACKAGEID="strus-0.0"

cd pkg/$PACKAGEID
dpkg-buildpackage

