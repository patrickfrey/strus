TOPDIR=../..
CPP_OBJS=\
	file.o \
	encode.o \
	malloc.o \
	dictionary.o \
	blocktable.o \
	blockcache.o \
	podvector.o \
	database.o \
	inserter.o \
	storage.o

CPP_INCS=\
	file.hpp \
	encode.hpp \
	malloc.hpp \
	dictionary.hpp \
	blocktable.hpp \
	blockcache.hpp \
	podvector.hpp \
	database.hpp \
	inserter.hpp

CPP_INCLUDE=-I/usr/local/include/ -I$(TOPDIR)/include -I.
CXXFLAGS=-g -Werror -Wall -pedantic -std=c++98
PATHFLAGS=-DSTRUS_DEFAULT_DBPATH="/home/patrick/"
SO_COMPILE_FLAGS=-fPIC
LDFLAGS=-L/usr/local/lib
LIBS=

%.o : %.cpp GNUmakefile $(CPP_INCS)
	g++ -c -o $@ $(SO_COMPILE_FLAGS) -DSHARED $(CXXFLAGS) $(CPP_INCLUDE) $(PATHFLAGS) $<

libstrus_filestorage.so: $(CPP_OBJS) $(CPP_INCS)
	g++ -shared -Wl,-soname,libstrus_filestorage.so.0 -o libstrus_filestorage.so.0.0.0 $(LDFLAGS) $(LIBS) $(CPP_OBJS)
	ln -sf libstrus_filestorage.so.0.0.0 libstrus_filestorage.so.0.0
	ln -sf libstrus_filestorage.so.0.0.0 libstrus_filestorage.so.0
	ln -sf libstrus_filestorage.so.0.0.0 libstrus_filestorage.so

all: libstrus_filestorage.so

.PHONY: clean
clean:
	rm -f ${CPP_OBJS}
	rm -f libstrus_filestorage.so
	rm -f libstrus_filestorage.so.0
	rm -f libstrus_filestorage.so.0.0
	rm -f libstrus_filestorage.so.0.0.0


