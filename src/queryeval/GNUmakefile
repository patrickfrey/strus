TOPDIR=../..
include $(TOPDIR)/variables.mk

CPP_OBJS=\
	parser/accumulateOperation.o \
	parser/joinFunction.o \
	parser/selectorExpression.o \
	parser/selectorSet.o \
	parser/summarizeOperation.o \
	parser/weightingFunction.o \
	parser/tupleGenerator.o \
	parser/lexems.o \
	accumulator.o \
	queryEval.o \
	queryEvalLib.o

CPP_INCLUDE=-I$(TOPDIR)/include -I$(TOPDIR)/include/private -I/usr/include -I.
CXXFLAGS=-g -Werror -Wall -pedantic $(OPTIMIZE) -std=c++98
LDFLAGS=-L/usr/lib
LIBS=

all: libstrus_queryeval.so

-include $(CPP_OBJS:.o=.d)

%.o : %.cpp GNUmakefile $(HFILES)
	g++ -fPIC -fvisibility=hidden -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp
	g++ -MM $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp > $*.d

libstrus_queryeval.so: $(CPP_OBJS)
	g++  -shared -o $@ $(CPP_OBJS) $(LDFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -f $(CPP_OBJS) $(CPP_OBJS:.o=.d) libstrus_queryeval.so

.PHONY: install
install:
	@mkdir -p $(INSTALL_LIBDIR)
	@cp libstrus_queryeval.so $(INSTALL_LIBDIR)

.PHONY: uninstall
uninstall:
	@rm $(INSTALL_LIBDIR)/libstrus_queryeval.so


