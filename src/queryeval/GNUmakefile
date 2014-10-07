TOPDIR=../..
CPP_OBJS=\
	queryParser.o \
	tupleGenerator.o \
	queryEvalLib.o

CPP_INCLUDE=-I$(TOPDIR)/include -I$(TOPDIR)/src/include -I/usr/include -I.
CXXFLAGS=-g -Werror -Wall -pedantic -std=c++98
LDFLAGS=-L/usr/lib
LIBS=

all: libstrus_queryeval.so

-include $(CPP_OBJS:.o=.d)

%.o : %.cpp GNUmakefile $(HFILES)
	g++ -fPIC -fvisibility=hidden -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp
	g++ -MM $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp > $*.d

libstrus_queryeval.so: $(CPP_OBJS)
	g++  -shared -o $@ $(CPP_OBJS) $(LDFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -f $(CPP_OBJS) $(CPP_OBJS:.o=.d) libstrus_queryeval.so


