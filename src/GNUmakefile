TOPDIR=..
CPP_OBJS=\
	iteratorIntersect.o \
	iteratorUnion.o \
	iteratorCutInRange.o \
	iterator.o \
	program.o

CPP_INCLUDE=-I$(TOPDIR)/include
CXXFLAGS=-g -Werror -Wall -pedantic -std=c++98
LDFLAGS=-L/usr/local/lib -L./filestorage
LIBS=-lstrus_filestorage

all: subdirs strus

%.o : %.cpp GNUmakefile
	g++ -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $<

strus: strus.o $(CPP_OBJS)
	g++ -o $@ $(CPP_OBJS) $< $(LDFLAGS) $(LIBS)

.PHONY: subdirs
subdirs:
	cd filestorage; make all; cd -

.PHONY: clean
clean:
	cd filestorage; make clean; cd -
	rm -f ${CPP_OBJS}
	rm -f strus.o
	rm -f strus

