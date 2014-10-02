TOPDIR=../..
CPP_OBJS=\
	indexPacker.o \
	storage.o \
	iterator.o \
	transaction.o \
	libstrus_lvdbstorage.o

CPP_INCLUDE=-I$(TOPDIR)/include -I/usr/include -I.
CXXFLAGS=-g -Werror -Wall -pedantic -std=c++98
LDFLAGS=-L/usr/lib
LIBS=-lleveldb

all: libstrus_lvdbstorage.so

%.o : %.cpp GNUmakefile
	g++ -fPIC -fvisibility=hidden -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $<

libstrus_lvdbstorage.so: $(CPP_OBJS)
	g++  -shared -o $@ $(CPP_OBJS) $(LDFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -f ${CPP_OBJS} 

