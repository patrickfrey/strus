TOPDIR=..
CPP_OBJS=\
	iterator/iteratorIntersect.o \
	iterator/iteratorUnion.o \
	iterator/iteratorCutInRange.o \
	queryeval/queryEvalStrus.o

CPP_INCLUDE=-I$(TOPDIR)/include
CXXFLAGS=-g -Werror -Wall -pedantic -std=c++98
LDFLAGS=-L/usr/local/lib -L./lvdbstorage
LIBS=-lstrus_lvdbstorage

all: subdirs strus

%.o : %.cpp GNUmakefile
	g++ -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $<

strus: strus.o $(CPP_OBJS)
	g++ -o $@ $(CPP_OBJS) $< $(LDFLAGS) $(LIBS)

.PHONY: subdirs
subdirs:
	cd lvdbstorage; make all; cd -

.PHONY: clean
clean:
	cd lvdbstorage; make clean; cd -
	rm -f ${CPP_OBJS}

