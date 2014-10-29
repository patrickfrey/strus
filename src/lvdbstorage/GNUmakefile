TOPDIR=../..
include $(TOPDIR)/variables.mk

CPP_OBJS=\
	indexPacker.o \
	storage.o \
	iterator.o \
	transaction.o \
	storageLib.o

CPP_INCLUDE=-I$(TOPDIR)/include -I$(TOPDIR)/include/private -I/usr/include -I.
CXXFLAGS=-g -Werror -Wall -pedantic $(OPTIMIZE) -std=c++98
MODFLAGS=-fPIC
#-fvisibility=hidden 
LDFLAGS=-L/usr/lib
LIBS=-lleveldb -lpthread -lstdc++ -Wl,--no-as-needed -lsnappy

all: libstrus_lvdbstorage.so

-include $(CPP_OBJS:.o=.d)

%.o : %.cpp GNUmakefile $(IFILES) $(HFILES)
	g++ $(MODFLAGS) -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp
	g++ -MM $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp > $*.d

libstrus_lvdbstorage.so: $(CPP_OBJS)
	g++  -shared -o $@ $(CPP_OBJS) $(LDFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -f $(CPP_OBJS) $(CPP_OBJS:.o=.d) libstrus_lvdbstorage.so

.PHONY: install
install:
	@mkdir -p $(INSTALL_LIBDIR)
	@cp libstrus_lvdbstorage.so $(INSTALL_LIBDIR)

.PHONY: uninstall
uninstall:
	@rm $(INSTALL_LIBDIR)/libstrus_lvdbstorage.so

