TOPDIR=../../..

CPP_OBJS=\
	randomCollectionTest.o

CPP_INCLUDE=-I$(TOPDIR)/include -I$(TOPDIR)/src -I$(TOPDIR)/include/private -I$(TOPDIR)/include/tests
CXXFLAGS=-g -Werror -Wall -pedantic -std=c++98
LDFLAGS=-L$(TOPDIR)/src/lvdbstorage -L$(TOPDIR)/src/queryeval -L$(TOPDIR)/src/queryproc -L/usr/lib
LIBS=-lstrus_lvdbstorage -lstrus_queryeval -lstrus_queryproc -lleveldb -lboost_system

all: randomCollectionTest

-include $(CPP_OBJS:.o=.d)

%.o : %.cpp GNUmakefile
	$(CXX) -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp
	$(CXX) -MM $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp > $*.d

randomCollectionTest: $(CPP_OBJS)
	$(CXX) -o $@ $(CPP_OBJS) $(LDFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -f $(CPP_OBJS) $(CPP_OBJS:.o=.d) randomCollectionTest

