TOPDIR=..

CPP_OBJS=\
	queryproc/iterator/iteratorIntersect.o \
	queryproc/iterator/iteratorUnion.o \
	queryproc/iterator/iteratorCutInRange.o \
	queryproc/queryProcessor.o \
	queryeval/tupleGenerator.o \
	queryeval/queryParser.o \
	queryeval/queryEval.o

CPP_INCLUDE=-I$(TOPDIR)/include -I$(TOPDIR)/src
CXXFLAGS=-g -Werror -Wall -pedantic -std=c++98
LDFLAGS=-L./lvdbstorage -L/usr/lib
LIBS=-lstrus_lvdbstorage -lleveldb -lboost_system

all: subdirs strus

%.o : %.cpp GNUmakefile
	g++ -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $<

strus: $(CPP_OBJS) strus.o
	g++ -o $@ strus.o $(CPP_OBJS) $(LDFLAGS) $(LIBS)

.PHONY: subdirs
subdirs:
	cd lvdbstorage; make all; cd -

.PHONY: clean
clean:
	cd lvdbstorage; make clean; cd -
	rm -f ${CPP_OBJS}

