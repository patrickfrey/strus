TOPDIR=../..
include $(TOPDIR)/variables.mk

CPP_OBJS=

CPP_INCLUDE=-I$(TOPDIR)/include -I$(TOPDIR)/include/private -I$(TOPDIR)/src/lvdbstorage
CXXFLAGS=-g -Werror -Wall -pedantic $(OPTIMIZE) -std=c++98
LDFLAGS=-L$(TOPDIR)/src/lvdbstorage -L/usr/lib
LIBS=-lstrus_lvdbstorage -lleveldb -lboost_system

all: strusInspect

-include $(CPP_OBJS:.o=.d)

%.o : %.cpp GNUmakefile
	$(CXX) -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp
	$(CXX) -MM $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp > $*.d

strusInspect: $(CPP_OBJS) strusInspect.o
	$(CXX) -o $@ strusInspect.o $(CPP_OBJS) $(LDFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -f ${CPP_OBJS} strusInspect strusInspect.o strusInspect.d

.PHONY: install
install:
	@mkdir -p $(INSTALL_BINDIR)
	@cp strusInspect $(INSTALL_BINDIR)

.PHONY: uninstall
uninstall:
	@rm $(INSTALL_BINDIR)/strusInspect



