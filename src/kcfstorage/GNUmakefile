TOPDIR=../..
CPP_OBJS=\
	file.o \
	encode.o \
	malloc.o \
	dictionary.o \
	blocktable.o \
	blockcache.o \
	podvector.o \
	keytable.o \
	database.o \
	kcfstorage.o

CPP_INCS=\
	file.hpp \
	encode.hpp \
	malloc.hpp \
	dictionary.hpp \
	blocktable.hpp \
	blockcache.hpp \
	keytable.hpp \
	database.hpp \
	podvector.hpp

CPP_INCLUDE=-I/usr/local/include/ -I$(TOPDIR)/include -I.
CXXFLAGS=-g -Werror -Wall -pedantic -std=c++98
PATHFLAGS=-DSTRUS_DEFAULT_DBPATH="/home/patrick/"
SO_COMPILE_FLAGS=-fPIC
LDFLAGS=-L/usr/local/lib
LIBS=-lkyotocabinet

%.o : %.cpp GNUmakefile $(CPP_INCS)
	g++ -c -o $@ $(SO_COMPILE_FLAGS) -DSHARED $(CXXFLAGS) $(CPP_INCLUDE) $(PATHFLAGS) $<

libstrus_kcfstorage.so: $(CPP_OBJS) $(CPP_INCS)
	g++ -shared -Wl,-soname,libstrus_kcfstorage.so.0 -o libstrus_kcfstorage.so.0.0.0 $(LDFLAGS) $(LIBS) $(CPP_OBJS)
	ln -sf libstrus_kcfstorage.so.0.0.0 libstrstrus_kcfstorage.so.0.0
	ln -sf libstrus_kcfstorage.so.0.0.0 libstrus_kcfstorage.so.0
	ln -sf libstrus_kcfstorage.so.0.0.0 libstrus_kcfstorage.so

all: libstrus_kcfstorage.so

.PHONY: clean
clean:
	rm -f ${CPP_OBJS}
	rm -f strus_kcfstorage.so
	rm -f strus_kcfstorage.so.1
	rm -f strus_kcfstorage.so.1.0


