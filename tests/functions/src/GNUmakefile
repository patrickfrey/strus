TOPDIR=../../..

CPP_OBJS=\
	functions.o

CPP_INCLUDE=-I$(TOPDIR)/include -I$(TOPDIR)/src/storage -I$(TOPDIR)/include/private -I$(TOPDIR)/include/tests
CXXFLAGS=-g -Werror -Wall -Wfatal-errors -pedantic -std=c++98
LDFLAGS=
LIBS=

all: functions

-include $(CPP_OBJS:.o=.d)

%.o : %.cpp GNUmakefile
	$(CXX) -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp
	$(CXX) -MM $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp > $*.d

functions: $(CPP_OBJS)
	$(CXX) -o $@ $(CPP_OBJS) $(LDFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -f $(CPP_OBJS) $(CPP_OBJS:.o=.d) functions

