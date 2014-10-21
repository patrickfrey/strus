TOPDIR=../..
LD_LIBRARY_PATH=$(TOPDIR)/tests/utils:$(TOPDIR)/src/lvdbstorage:$(TOPDIR)/src/queryproc:$(TOPDIR)/src/queryeval

all:
	cd src; make all; cd ..

test: all
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH) src/parseQueryEvalProgramTest data/test_complex_program.prg

clean:
	cd src; make clean; cd ..


