TOPDIR=../../..

CPP_OBJS=\
	parseQueryEvalProgramTest.o

CPP_INCLUDE=-I$(TOPDIR)/include -I$(TOPDIR)/src -I$(TOPDIR)/include/tests -I$(TOPDIR)/include/private
CXXFLAGS=-g -Werror -Wall -pedantic -std=c++98
LDFLAGS=-L$(TOPDIR)/src/lvdbstorage -L$(TOPDIR)/src/queryeval -L$(TOPDIR)/src/queryproc -L$(TOPDIR)/tests/utils -L/usr/lib
LIBS=-lstrus_lvdbstorage -lstrus_queryeval -lstrus_queryproc -lstrus_testutils -lleveldb -lboost_system

all: parseQueryEvalProgramTest

-include $(CPP_OBJS:.o=.d)

%.o : %.cpp GNUmakefile
	g++ -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp
	g++ -MM $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp > $*.d

parseQueryEvalProgramTest: $(CPP_OBJS)
	g++ -o $@ $(CPP_OBJS) $(LDFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -f $(CPP_OBJS) $(CPP_OBJS:.o=.d) parseQueryEvalProgramTest

