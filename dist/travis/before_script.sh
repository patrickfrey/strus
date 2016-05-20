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
		if test "X$CC" = "Xgcc"; then
			brew install gcc48 --enable-all-languages || true
			brew link --force gcc48 || true
		fi
		brew install \
			cmake \
			boost \
			gettext \
			snappy \
			leveldb \
			|| true
		# make sure cmake finds the brew version of gettext
		brew link --force gettext || true
		brew link leveldb || true
		brew link snappy || true
		# rebuild leveldb to use gcc-4.8 ABI on OSX (libstc++ differs
		# from stdc++, leveldb uses std::string in API functions, C
		# libraries like gettext and snappy and even boost do not 
		# have this problem)
		if test "X$CC" = "Xgcc"; then
			brew reinstall leveldb --cc=gcc-4.8
		fi
		;;
	
	*)
		echo "ERROR: unknown operating system '$OS'."
		;;
esac

