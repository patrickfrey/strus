TOPDIR=../../..

CPP_OBJS=\
	createRandomTestCollection.o

CPP_INCLUDE=-I$(TOPDIR)/include -I$(TOPDIR)/src
CXXFLAGS=-g -Werror -Wall -pedantic -std=c++98
LDFLAGS=-L$(TOPDIR)/lvdbstorage -L/usr/lib
LIBS=-lstrus_lvdbstorage -lleveldb -lboost_system

all: subdirs strus

%.o : %.cpp GNUmakefile
	g++ -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $<

strus: $(CPP_OBJS)
	g++ -o $@ $(CPP_OBJS) $(LDFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -f ${CPP_OBJS}

