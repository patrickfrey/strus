TOPDIR=../../..
include $(TOPDIR)/variables.mk

CPP_OBJS=\
	testMetaDataRestrictions.o \
	testMetaDataRestrictionFiles.o

CPP_INCLUDE=-I$(TOPDIR)/include -I$(TOPDIR)/src/utils -I$(TOPDIR)/src/queryeval -I$(TOPDIR)/src/storage -I$(TOPDIR)/include/private
CXXFLAGS=-g -Werror -Wall -Wfatal-errors -pedantic $(OPTIMIZE) -std=c++98
LDFLAGS=-L$(TOPDIR)/src/database_leveldb
LIBS=-lstrus_database_leveldb

all: testMetaDataRestrictions

-include $(CPP_OBJS:.o=.d)

%.o : %.cpp GNUmakefile
	$(CXX) -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp
	$(CXX) -MM $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp > $*.d

testMetaDataRestrictions: $(CPP_OBJS)
	$(CXX) -o $@ $(CPP_OBJS) $(LDFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -f $(CPP_OBJS) $(CPP_OBJS:.o=.d) testMetaDataRestrictions

