TOPDIR=../../..

CPP_OBJS=\
	randomCollectionTest.o

CPP_INCLUDE=-I$(TOPDIR)/include -I$(TOPDIR)/src
CXXFLAGS=-g -Werror -Wall -pedantic -std=c++98
LDFLAGS=-L$(TOPDIR)/src/lvdbstorage -L$(TOPDIR)/src/queryeval -L$(TOPDIR)/src/queryproc -L/usr/lib
LIBS=-lstrus_lvdbstorage -lstrus_queryeval -lstrus_queryproc -lleveldb -lboost_system

all: randomCollectionTest

%.o : %.cpp GNUmakefile
	g++ -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $<

randomCollectionTest: $(CPP_OBJS)
	g++ -o $@ $(CPP_OBJS) $(LDFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -f ${CPP_OBJS} randomCollectionTest

