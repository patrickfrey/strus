TOPDIR=../..
include $(TOPDIR)/variables.mk

CPP_OBJS=\
	iterator/iteratorIntersect.o \
	iterator/iteratorUnion.o \
	iterator/iteratorDifference.o \
	iterator/iteratorStructWithin.o \
	iterator/iteratorStructSequence.o \
	iterator/iteratorHelpers.o \
	weighting/weightingBM25.o \
	weighting/weightingIdfBased.o \
	summarizer/summarizerMatchPhrase.o \
	summarizer/summarizerMetaData.o \
	queryProcessor.o \
	queryProcessorLib.o

HFILES= queryProcessor.hpp
CPP_INCLUDE=-I$(TOPDIR)/include -I$(TOPDIR)/include/private -I$(TOPDIR)/3rdParty/fastapprox/include/ -I/usr/include -I.
CXXFLAGS=-g -Werror -Wall -pedantic $(OPTIMIZE) -std=c++98
LDFLAGS=-L/usr/lib
LIBS=

all: libstrus_queryproc.so

-include $(CPP_OBJS:.o=.d)

%.o : %.cpp GNUmakefile $(HFILES)
	g++ -fPIC $(VISIBILITY) -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp
	g++ -MM $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp > $*.d

libstrus_queryproc.so: $(CPP_OBJS)
	g++  -shared -o $@ $(CPP_OBJS) $(LDFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -f $(CPP_OBJS) $(CPP_OBJS:.o=.d) libstrus_queryproc.so


.PHONY: install
install:
	@mkdir -p $(INSTALL_LIBDIR)
	@cp libstrus_queryproc.so $(INSTALL_LIBDIR)

.PHONY: uninstall
uninstall:
	@rm $(INSTALL_LIBDIR)/libstrus_queryproc.so


