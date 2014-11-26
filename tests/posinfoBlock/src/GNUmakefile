TOPDIR=../../..

CPP_OBJS=\
	testPosinfoBlock.o

CPP_INCLUDE=-I$(TOPDIR)/include -I$(TOPDIR)/src -I$(TOPDIR)/include/private -I$(TOPDIR)/include/tests
CXXFLAGS=-g -Werror -Wall -pedantic -std=c++98
LDFLAGS=
LIBS=

all: testPosinfoBlock

-include $(CPP_OBJS:.o=.d)

%.o : %.cpp GNUmakefile
	$(CXX) -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp
	$(CXX) -MM $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp > $*.d

testPosinfoBlock: $(CPP_OBJS)
	$(CXX) -o $@ $(CPP_OBJS) $(LDFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -f $(CPP_OBJS) $(CPP_OBJS:.o=.d) testPosinfoBlock

