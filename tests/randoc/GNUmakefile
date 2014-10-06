TOPDIR=../..
LD_LIBRARY_PATH=$(TOPDIR)/src/lvdbstorage:$(TOPDIR)/src/queryproc:$(TOPDIR)/src/queryeval

all:
	cd src; make all; cd ..
test: all
	@rm -Rf data/test
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH) src/createRandomTestCollection "path=data/test" 100 100 50
clean:
	cd src; make clean; cd ..
	rm -Rf data/teststorage

