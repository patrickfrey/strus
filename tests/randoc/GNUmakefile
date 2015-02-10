TOPDIR=../..
LD_LIBRARY_PATH=$(TOPDIR)/src/utils:$(TOPDIR)/src/database_leveldb:$(TOPDIR)/src/storage:$(TOPDIR)/src/queryproc:$(TOPDIR)/src/queryeval

all:
	cd src; make all; cd ..

test: all dataclean
	mkdir -p data
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH) src/testRandomCollection "path=data/test" 200 200 100 100

clean: dataclean
	cd src; make clean; cd ..

dataclean:
	@rm -Rf data/test/

