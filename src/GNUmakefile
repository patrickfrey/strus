TOPDIR=..
CPP_OBJS=\
	iterator.o \
	operation.o

CPP_INCLUDE=-I$(TOPDIR)/include
CXXFLAGS=-Werror -Wall -pedantic -std=c++98
LDFLAGS=-L/usr/local/lib
LIBS=

%.o : %.cpp GNUmakefile
	g++ -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $<

strus: strus.o $(CPP_OBJS)
	g++ -o $@ $(CPP_OBJS) $< $(LDFLAGS) $(LIBS)

all: strus

.PHONY: clean
clean:
	rm -f ${CPP_OBJS}
	rm -f strus.o
	rm -f strus



