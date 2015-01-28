TOPDIR=../..
include $(TOPDIR)/variables.mk

CPP_OBJS=\
	storage.o \
	storageLib.o \
	keyValueStorage.o \
	varSizeNodeTree.o \
	statistics.o \
	floatConversions.o \
	indexPacker.o \
	databaseKey.o \
	dataBlock.o \
	dataBlockStorage.o \
	invTermBlock.o \
	posinfoBlock.o \
	posinfoBlockMap.o \
	posinfoIterator.o \
	forwardIndexBlock.o \
	forwardIndexBlockMap.o \
	booleanBlock.o \
	booleanBlockMap.o \
	userAclBlockMap.o \
	metaDataElement.o \
	metaDataDescription.o \
	metaDataRecord.o \
	metaDataBlock.o \
	metaDataBlockMap.o \
	metaDataBlockCache.o \
	attributeReader.o \
	attributeMap.o \
	keyMap.o \
	documentFrequencyMap.o \
	postingIterator.o \
	indexSetIterator.o \
	nullIterator.o \
	forwardIterator.o \
	storageDocument.o \
	storageDocumentChecker.o \
	storageTransaction.o \
	storageAlterMetaDataTable.o

PRG_OBJS=\
	extractKeyValueData.o

CPP_INCLUDE=\
	-I$(TOPDIR)/include \
	-I$(TOPDIR)/include/private \
	-I.

CXXFLAGS=-g -Wfatal-errors -Werror -Wall -pedantic $(OPTIMIZE) -std=c++98 -msse4.2
MODFLAGS=-fPIC $(VISIBILITY)
LDFLAGS=-L/usr/lib -L$(TOPDIR)/src/utils -L$(TOPDIR)/src/database_leveldb
LIBS=-lstrus_utils -lstrus_database_leveldb -lpthread -lstdc++ -Wl,--no-as-needed

all: libstrus_storage.so strusCheckStorage strusDumpStorage

-include $(CPP_OBJS:.o=.d)
-include $(PRG_OBJS:.o=.d)

%.o : %.cpp GNUmakefile $(IFILES) $(HFILES)
	$(CXX) $(MODFLAGS) -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp
	$(CXX) -MM $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp > $*.d

libstrus_storage.so: $(CPP_OBJS)
	$(CXX) -shared -o $@ $(CPP_OBJS) $(LDFLAGS) $(LIBS)

strusCheckStorage: $(CPP_OBJS) $(PRG_OBJS) strusCheckStorage.o
	$(CXX) -o $@ strusCheckStorage.o $(PRG_OBJS) $(CPP_OBJS) $(LDFLAGS) $(LIBS) -lboost_system

strusDumpStorage: $(CPP_OBJS) $(PRG_OBJS) strusDumpStorage.o
	$(CXX) -o $@ strusDumpStorage.o $(PRG_OBJS) $(CPP_OBJS) $(LDFLAGS) $(LIBS) -lboost_system

.PHONY: clean
clean:
	@rm -f $(CPP_OBJS) $(CPP_OBJS:.o=.d)
	@rm -f $(PRG_OBJS) $(PRG_OBJS:.o=.d)
	@rm -f libstrus_storage.so
	@rm -f strusCheckStorage strusDumpStorage
	@rm -f strusCheckStorage.o strusDumpStorage.o
	@rm -f strusCheckStorage.d strusDumpStorage.d

.PHONY: install
install:
	@mkdir -p $(INSTALL_LIBDIR)
	@cp libstrus_storage.so $(INSTALL_LIBDIR)
	@cp strusCheckStorage $(INSTALL_BINDIR)
	@cp strusDumpStorage $(INSTALL_BINDIR)
 
.PHONY: uninstall
uninstall:
	@rm -f $(INSTALL_LIBDIR)/libstrus_storage.so
	@rm -f $(INSTALL_BINDIR)/strusCheckStorage
	@rm -f $(INSTALL_BINDIR)/strusDumpStorage

