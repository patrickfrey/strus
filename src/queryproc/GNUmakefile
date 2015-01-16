TOPDIR=../..
include $(TOPDIR)/variables.mk

CPP_OBJS=\
	iterator/postingIteratorIntersect.o \
	iterator/postingIteratorUnion.o \
	iterator/postingIteratorUnionWeighted.o \
	iterator/postingIteratorDifference.o \
	iterator/postingIteratorStructWithin.o \
	iterator/postingIteratorStructSequence.o \
	iterator/postingIteratorHelpers.o \
	weighting/weightingBM25.o \
	summarizer/summarizerMatchPhrase.o \
	summarizer/summarizerListMatches.o \
	summarizer/summarizerAttribute.o \
	summarizer/summarizerMetaData.o \
	queryProcessor.o \
	queryProcessorLib.o

HFILES= queryProcessor.hpp
CPP_INCLUDE=-I$(TOPDIR)/include -I$(TOPDIR)/include/private -I$(TOPDIR)/3rdParty/fastapprox/include/ -I.
CXXFLAGS=-g -Werror -Wall -pedantic $(OPTIMIZE) -std=c++98
LDFLAGS=-L/usr/lib
LIBS=

all: libstrus_queryproc.so

-include $(CPP_OBJS:.o=.d)

%.o : %.cpp GNUmakefile $(HFILES)
	$(CXX) -fPIC $(VISIBILITY) -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp
	$(CXX) -MM $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp > $*.d

libstrus_queryproc.so: $(CPP_OBJS)
	$(CXX)  -shared -o $@ $(CPP_OBJS) $(LDFLAGS) $(LIBS)

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


