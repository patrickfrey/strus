TOPDIR=../..
include $(TOPDIR)/variables.mk

CPP_OBJS=\
	indexPacker.o \
	databaseKey.o \
	metaDataBlock.o \
	metaDataReader.o \
	storage.o \
	iterator.o \
	nullIterator.o \
	forwardIndexViewer.o \
	transaction.o \
	storageLib.o

CPP_INCLUDE=-I$(TOPDIR)/include -I$(TOPDIR)/include/private -I.
CXXFLAGS=-g -Werror -Wall -pedantic $(OPTIMIZE) -std=c++98
MODFLAGS=-fPIC $(VISIBILITY)
LDFLAGS=-L/usr/lib
LIBS=-lleveldb -lpthread -lstdc++ -Wl,--no-as-needed -lsnappy

all: libstrus_lvdbstorage.so strusCheckStorage

-include $(CPP_OBJS:.o=.d)

%.o : %.cpp GNUmakefile $(IFILES) $(HFILES)
	$(CXX) $(MODFLAGS) -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp
	$(CXX) -MM $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp > $*.d

libstrus_lvdbstorage.so: $(CPP_OBJS)
	$(CXX) -shared -o $@ $(CPP_OBJS) $(LDFLAGS) $(LIBS)

strusCheckStorage: $(CPP_OBJS) strusCheckStorage.o
	$(CXX) -o $@ strusCheckStorage.o $(CPP_OBJS) $(LDFLAGS) $(LIBS) -lboost_system

.PHONY: clean
clean:
	rm -f $(CPP_OBJS) $(CPP_OBJS:.o=.d) libstrus_lvdbstorage.so

.PHONY: install
install:
	@mkdir -p $(INSTALL_LIBDIR)
	@cp libstrus_lvdbstorage.so $(INSTALL_LIBDIR)
	@cp strusCheckStorage $(INSTALL_BINDIR)

.PHONY: uninstall
uninstall:
	@rm $(INSTALL_LIBDIR)/libstrus_lvdbstorage.so
	@rm $(INSTALL_BINDIR)/strusCheckStorage

