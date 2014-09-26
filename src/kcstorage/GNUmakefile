TOPDIR=../..
CPP_OBJS=\
	indexPacker.o \
	kcstorage.o

CPP_INCLUDE=-I$(TOPDIR)/include -I/usr/local/include -I.
CXXFLAGS=-g -Werror -Wall -pedantic -std=c++98
LDFLAGS=-L/usr/local/lib
LIBS=-lkyotocabinet

all: libstrus_kcstorage.so

%.o : %.cpp GNUmakefile
	g++ -fPIC -fvisibility=hidden -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $<

libstrus_kcstorage.so: $(CPP_OBJS)
	g++  -shared -o $@ $(CPP_OBJS) $(LDFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -f ${CPP_OBJS} 

