TOPDIR=../../..
include $(TOPDIR)/variables.mk

CPP_OBJS=\
	testVarSizeNodeTree.o

CPP_INCLUDE=-I$(TOPDIR)/include -I$(TOPDIR)/src/lvdbstorage -I$(TOPDIR)/include/private
CXXFLAGS=-g -Werror -Wall -pedantic $(OPTIMIZE) -std=c++98
LDFLAGS=
LIBS=

all: testVarSizeNodeTree

-include $(CPP_OBJS:.o=.d)

%.o : %.cpp GNUmakefile
	$(CXX) -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp
	$(CXX) -MM $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp > $*.d

testVarSizeNodeTree: $(CPP_OBJS)
	$(CXX) -o $@ $(CPP_OBJS) $(LDFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -f $(CPP_OBJS) $(CPP_OBJS:.o=.d) testVarSizeNodeTree

