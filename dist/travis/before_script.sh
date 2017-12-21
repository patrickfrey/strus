#!/bin/sh

set -e

OS=$(uname -s)

case $OS in
	Linux)
		sudo apt-get update -qq
		sudo apt-get install -y \
			cmake \
			libboost-all-dev \
			libleveldb-dev
		;;
		
	Darwin)
		brew update
		brew upgrade cmake
		brew upgrade boost
		brew install gettext snappy leveldb || true
		# make sure cmake finds the brew version of gettext
		brew link --force gettext || true
		brew link leveldb || true
		brew link snappy || true
		;;

	*)
		echo "ERROR: unknown operating system '$OS'."
		;;
esac

