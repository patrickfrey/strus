TOPDIR=../..
CPP_OBJS=\
	kcfdb.o \
	kcfstorage.o

CPP_INCLUDE=-I$(TOPDIR)/include -I.
CXXFLAGS=-Werror -Wall -pedantic -std=c++98
PATHFLAGS=-DSTRUS_DEFAULT_DBPATH="/home/patrick/"
SO_COMPILE_FLAGS=-fPIC
LDFLAGS=-L/usr/local/lib
LIBS=-lkyotocabinet

%.o : %.cpp GNUmakefile
	g++ -c -o $@ $(SO_COMPILE_FLAGS) -DSHARED $(CXXFLAGS) $(CPP_INCLUDE) $(PATHFLAGS) $<

kcfstorage.so: $(CPP_OBJS)
	g++ -shared -o $@ $(LDFLAGS) $(LIBS) $(CPP_OBJS) $<

all: kcfstorage.so

.PHONY: clean
clean:
	rm -f ${CPP_OBJS}
	rm -f kcfstorage.so


