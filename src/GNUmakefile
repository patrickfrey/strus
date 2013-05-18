TOPDIR=..
CPP_OBJS=\
	iterator.o \
	program.o

CPP_INCLUDE=-I$(TOPDIR)/include
CXXFLAGS=-g -Werror -Wall -pedantic -std=c++98
LDFLAGS=-L/usr/local/lib -L./kcfstorage
LIBS=-lstrus_kcfstorage -lkyotocabinet

all: subdirs strus

%.o : %.cpp GNUmakefile
	g++ -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $<

strus: strus.o $(CPP_OBJS)
	g++ -o $@ $(CPP_OBJS) $< $(LDFLAGS) $(LIBS)

.PHONY: subdirs
subdirs:
	cd kcfstorage; make all; cd -

.PHONY: clean
clean:
	cd kcfstorage; make clean; cd -
	rm -f ${CPP_OBJS}
	rm -f strus.o
	rm -f strus

