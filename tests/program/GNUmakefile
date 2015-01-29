TOPDIR=../..
LD_LIBRARY_PATH=$(TOPDIR)/src/utils:$(TOPDIR)/src/database_leveldb:$(TOPDIR)/src/storage:$(TOPDIR)/src/queryproc:$(TOPDIR)/src/queryeval

all:
	cd src; make all; cd ..

test: all
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH) src/parseQueryEvalProgramTest data/test_complex_program.prg

clean:
	cd src; make clean; cd ..


