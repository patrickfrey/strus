TOPDIR=../..
include $(TOPDIR)/variables.mk

CPP_OBJS=

CPP_INCLUDE=-I$(TOPDIR)/include -I$(TOPDIR)/include/private
CXXFLAGS=-g -Werror -Wall -pedantic $(OPTIMIZE) -std=c++98
LDFLAGS=-L$(TOPDIR)/src/lvdbstorage -L$(TOPDIR)/src/queryeval -L$(TOPDIR)/src/queryproc -L/usr/lib
LIBS=-lstrus_lvdbstorage -lstrus_queryeval -lstrus_queryproc -lleveldb -lboost_system

all: strusCreate

-include $(CPP_OBJS:.o=.d)

%.o : %.cpp GNUmakefile
	$(CXX) -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp
	$(CXX) -MM $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp > $*.d

strusCreate: $(CPP_OBJS) strusCreate.o
	$(CXX) -o $@ strusCreate.o $(CPP_OBJS) $(LDFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -f ${CPP_OBJS} strusCreate strusCreate.o strusCreate.d

.PHONY: install
install:
	@mkdir -p $(INSTALL_BINDIR)
	@cp strusCreate $(INSTALL_BINDIR)

.PHONY: uninstall
uninstall:
	@rm $(INSTALL_BINDIR)/strusCreate



