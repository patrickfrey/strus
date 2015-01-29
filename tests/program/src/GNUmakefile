TOPDIR=../../..

CPP_OBJS=\
	parseQueryEvalProgramTest.o

CPP_INCLUDE=-I$(TOPDIR)/include -I$(TOPDIR)/src -I$(TOPDIR)/include/tests -I$(TOPDIR)/include/private
CXXFLAGS=-g -Werror -Wall -Wfatal-errors -pedantic $(OPTIMIZE) -std=c++98
LDFLAGS=-L$(TOPDIR)/src/database_leveldb -L$(TOPDIR)/src/storage -L$(TOPDIR)/src/queryeval -L$(TOPDIR)/src/queryproc -L$(TOPDIR)/src/utils -L/usr/lib
LIBS=-lstrus_database_leveldb -lstrus_storage -lstrus_queryeval -lstrus_queryproc -lstrus_utils -lleveldb -lboost_system

all: parseQueryEvalProgramTest

-include $(CPP_OBJS:.o=.d)

%.o : %.cpp GNUmakefile
	$(CXX) -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp
	$(CXX) -MM $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp > $*.d

parseQueryEvalProgramTest: $(CPP_OBJS)
	$(CXX) -o $@ $(CPP_OBJS) $(LDFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -f $(CPP_OBJS) $(CPP_OBJS:.o=.d) parseQueryEvalProgramTest

