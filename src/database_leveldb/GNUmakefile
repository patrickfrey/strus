TOPDIR=../..
include $(TOPDIR)/variables.mk

CPP_OBJS=\
	databaseLib.o \
	database.o \
	databaseCursor.o \
	databaseTransaction.o

CPP_INCLUDE=\
	-I$(TOPDIR)/include \
	-I$(TOPDIR)/include/private \
	-I.

CXXFLAGS=-g -Wfatal-errors -Werror -Wall -pedantic $(OPTIMIZE) -std=c++98 -msse4.2
MODFLAGS=-fPIC $(VISIBILITY)
LDFLAGS=-L/usr/lib
LIBS=-lleveldb -lpthread -lstdc++ -Wl,--no-as-needed -lsnappy

all: libstrus_database_leveldb.so

-include $(CPP_OBJS:.o=.d)
-include $(PRG_OBJS:.o=.d)

%.o : %.cpp GNUmakefile $(IFILES) $(HFILES)
	$(CXX) $(MODFLAGS) -c -o $@ $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp
	$(CXX) -MM $(CXXFLAGS) $(CPP_INCLUDE) $*.cpp > $*.d

libstrus_database_leveldb.so: $(CPP_OBJS)
	$(CXX) -shared -o $@ $(CPP_OBJS) $(LDFLAGS) $(LIBS)

.PHONY: clean
clean:
	@rm -f $(CPP_OBJS) $(CPP_OBJS:.o=.d)
	@rm -f $(PRG_OBJS) $(PRG_OBJS:.o=.d)
	@rm -f libstrus_database_leveldb.so

.PHONY: install
install:
	@mkdir -p $(INSTALL_LIBDIR)
	@cp libstrus_database_leveldb.so $(INSTALL_LIBDIR)
 
.PHONY: uninstall
uninstall:
	@rm -f $(INSTALL_LIBDIR)/libstrus_database_leveldb.so

