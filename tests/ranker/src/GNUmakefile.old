TOPDIR=../../..
include $(TOPDIR)/variables.mk

CPP_OBJS=\
	testRanker.o

CPP_INCLUDE=-I$(TOPDIR)/include -I$(TOPDIR)/src/queryeval -I$(TOPDIR)/include/private
CXXFLAGS=-g -Werror -Wall -Wfatal-errors -pedantic $(OPTIMIZE) -std=c++98
LDFLAGS=
LIBS=

all: testRanker

-include $(CPP_OBJS:.o=.d)

%.o : %.cpp GNUmakefile
	$(CXX) -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp
	$(CXX) -MM $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp > $*.d

testRanker: $(CPP_OBJS)
	$(CXX) -o $@ $(CPP_OBJS) $(LDFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -f $(CPP_OBJS) $(CPP_OBJS:.o=.d) testRanker

