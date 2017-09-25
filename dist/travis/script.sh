#!/bin/sh

set -e

OS=$(uname -s)

PROJECT=strus

build_strus_project() {
	prj_cmakeflags=$1
	case $OS in
		Linux)
			mkdir build
			cd build
			cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release \
				-DLIB_INSTALL_DIR=lib -DCMAKE_CXX_FLAGS=-g $prj_cmakeflags \
				..
			make VERBOSE=1
			make VERBOSE=1 CTEST_OUTPUT_ON_FAILURE=1 test
			sudo make VERBOSE=1 install
			cd ..
			;;
	
		Darwin)
			if test "X$CC" = "Xgcc"; then
				# gcc on OSX is a mere frontend to clang, force using gcc 4.8
				export CXX=g++-4.8
				export CC=gcc-4.8
				# forcing brew versions (of gettext) over Mac versions
				export CFLAGS=-I/usr/local
				export CXXFLAGS=-I/usr/local
				export LDFLAGS=-L/usr/local/lib
				mkdir build
				cd build
				cmake \
					-DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=Release \
					-DCMAKE_CXX_FLAGS=-g -G 'Unix Makefiles' $prj_cmakeflags \
					..
				make VERBOSE=1
				make VERBOSE=1 CTEST_OUTPUT_ON_FAILURE=1 test
				sudo make VERBOSE=1 install
				cd ..
			else
				# forcing brew versions (of gettext) over Mac versions
				export CFLAGS=-I/usr/local
				export CXXFLAGS=-I/usr/local
				export LDFLAGS=-L/usr/local/lib
				mkdir build
				cd build
				cmake \
					-DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=Release \
					-DCMAKE_CXX_FLAGS='-g -Wno-error=format-nonliteral -Wno-error=format-security' -G Xcode $prj_cmakeflags \
					..
				xcodebuild -configuration Release -target ALL_BUILD
				xcodebuild -configuration Release -target RUN_TESTS
				sudo xcodebuild -configuration Release -target install
				cd ..
			fi
			;;
			
		*)
			echo "ERROR: unknown operating system '$OS'."
			;;
	esac
}

setup_env() {
	case $OS in
		Linux)
			;;
		
		Darwin)
			if test "X$CC" = "Xgcc"; then
				# gcc on OSX is a mere frontend to clang, force using gcc 4.8
				export CXX=g++-4.8
				export CC=gcc-4.8
			fi
			# forcing brew versions (of gettext) over Mac versions
			export CFLAGS="-I/usr/local"
			export CXXFLAGS="-I/usr/local"
			export LDFLAGS="-L/usr/local/lib"
			;;
	
		*)
			echo "ERROR: unknown operating system '$OS'."
			;;
	esac
}

# set up environment
setup_env

# build pre-requisites
DEPS="strusBase"

GITURL=`git config remote.origin.url`
cd ..
for i in $DEPS; do
	git clone `echo $GITURL | sed "s@/$PROJECT\.@/$i.@g"` $i
	cd $i
	git submodule update --init --recursive
	git submodule foreach --recursive git checkout master
	git submodule foreach --recursive git pull

	build_strus_project ""
	cd ..
done

# build the package itself
cd $PROJECT
build_strus_project ""
cd ..

