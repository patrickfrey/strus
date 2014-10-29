TOPDIR=../..
include $(TOPDIR)/variables.mk

CPP_OBJS=

CPP_INCLUDE=-I$(TOPDIR)/include -I$(TOPDIR)/include/private
CXXFLAGS=-g -Werror -Wall -pedantic -O3 -std=c++98
LDFLAGS=-L$(TOPDIR)/src/lvdbstorage -L$(TOPDIR)/src/queryeval -L$(TOPDIR)/src/queryproc -L/usr/lib
LIBS=-lstrus_lvdbstorage -lstrus_queryeval -lstrus_queryproc -lleveldb -lboost_system

all: strus

-include $(CPP_OBJS:.o=.d)

%.o : %.cpp GNUmakefile
	g++ -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp
	g++ -MM $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp > $*.d

strus: $(CPP_OBJS) strus.o
	g++ -o $@ strus.o $(CPP_OBJS) $(LDFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -f ${CPP_OBJS} strus strus.o strus.d

.PHONY: install
install:
	@mkdir -p $(INSTALL_BINDIR)
	@cp strus $(INSTALL_BINDIR)

.PHONY: uninstall
uninstall:
	@rm $(INSTALL_BINDIR)/strus



