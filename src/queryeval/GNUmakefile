TOPDIR=../..
CPP_OBJS=\
	queryParser.o \
	tupleGenerator.o \
	libstrus_queryeval.o

CPP_INCLUDE=-I$(TOPDIR)/include -I$(TOPDIR)/src/include -I/usr/include -I.
CXXFLAGS=-g -Werror -Wall -pedantic -std=c++98
LDFLAGS=-L/usr/lib
LIBS=

all: libstrus_queryeval.so

%.o : %.cpp GNUmakefile
	g++ -fPIC -fvisibility=hidden -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $<

libstrus_queryeval.so: $(CPP_OBJS)
	g++  -shared -o $@ $(CPP_OBJS) $(LDFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -f ${CPP_OBJS} libstrus_queryeval.so

