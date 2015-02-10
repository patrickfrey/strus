TOPDIR=../..
include $(TOPDIR)/variables.mk

CPP_OBJS=\
	accumulator.o \
	metaDataRestriction.o \
	weightingConfig.o \
	summarizerConfig.o \
	query.o \
	queryEval.o \
	libstrus_queryeval.o

CPP_INCLUDE=-I$(TOPDIR)/include -I$(TOPDIR)/include/private -I.
CXXFLAGS=-g -Wfatal-errors -Werror -Wall -pedantic $(OPTIMIZE) -std=c++98
LDFLAGS=-L/usr/lib -L../utils
LIBS=-lstrus_utils

all: libstrus_queryeval.so

-include $(CPP_OBJS:.o=.d)

%.o : %.cpp GNUmakefile $(HFILES)
	$(CXX) -fPIC $(VISIBILITY) -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp
	$(CXX) -MM $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp > $*.d

libstrus_queryeval.so: $(CPP_OBJS)
	$(CXX)  -shared -o $@ $(CPP_OBJS) $(LDFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -f $(CPP_OBJS) $(CPP_OBJS:.o=.d) libstrus_queryeval.so

.PHONY: install
install:
	@mkdir -p $(INSTALL_LIBDIR)
	@cp libstrus_queryeval.so $(INSTALL_LIBDIR)

.PHONY: uninstall
uninstall:
	@rm -f $(INSTALL_LIBDIR)/libstrus_queryeval.so


