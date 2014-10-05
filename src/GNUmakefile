TOPDIR=..

CPP_OBJS=

all: subdirs

.PHONY: subdirs
subdirs:
	cd lvdbstorage; make all; cd -
	cd queryeval; make all; cd -
	cd queryproc; make all; cd -
	cd strus; make all; cd -

.PHONY: clean
clean:
	cd lvdbstorage; make clean; cd -
	cd queryeval; make clean; cd -
	cd queryproc; make clean; cd -
	cd strus; make clean; cd -


