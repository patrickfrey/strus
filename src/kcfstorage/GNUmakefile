TOPDIR=../..
CPP_OBJS=\
	file.o \
	blktable.o \
	persistentlist.o \
	keytable.o \
	database.o \
	kcfstorage.o

CPP_INCS=\
	file.hpp \
	blktable.hpp \
	keytable.hpp \
	database.hpp \
	persistentlist.hpp

CPP_INCLUDE=-I$(TOPDIR)/include -I.
CXXFLAGS=-g -Werror -Wall -pedantic -std=c++98
PATHFLAGS=-DSTRUS_DEFAULT_DBPATH="/home/patrick/"
SO_COMPILE_FLAGS=-fPIC
LDFLAGS=-L/usr/local/lib
LIBS=-lkyotocabinet

%.o : %.cpp GNUmakefile $(CPP_INCS)
	g++ -c -o $@ $(SO_COMPILE_FLAGS) -DSHARED $(CXXFLAGS) $(CPP_INCLUDE) $(PATHFLAGS) $<

libkcfstorage.so: $(CPP_OBJS) $(CPP_INCS)
	g++ -shared -Wl,-soname,libkcfstorage.so.0 -o libkcfstorage.so.0.0.0 $(LDFLAGS) $(LIBS) $(CPP_OBJS)
	ln -sf libkcfstorage.so.0.0.0 libkcfstorage.so.0.0
	ln -sf libkcfstorage.so.0.0.0 libkcfstorage.so.0
	ln -sf libkcfstorage.so.0.0.0 libkcfstorage.so

all: libkcfstorage.so

.PHONY: clean
clean:
	rm -f ${CPP_OBJS}
	rm -f kcfstorage.so
	rm -f kcfstorage.so.1
	rm -f kcfstorage.so.1.0


