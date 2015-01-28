TOPDIR=..
include $(TOPDIR)/variables.mk

CPP_OBJS=

all: subdirs

.PHONY: subdirs
subdirs:
	cd utils; make all; cd -
	cd database_leveldb; make all; cd -
	cd lvdbstorage; make all; cd -
	cd queryeval; make all; cd -
	cd queryproc; make all; cd -

.PHONY: clean
clean:
	cd utils; make clean; cd -
	cd database_leveldb; make clean; cd -
	cd lvdbstorage; make clean; cd -
	cd queryeval; make clean; cd -
	cd queryproc; make clean; cd -

.PHONY: install
install:
	cd utils; make install; cd -
	cd database_leveldb; make install; cd -
	cd lvdbstorage; make install; cd -
	cd queryeval; make install; cd -
	cd queryproc; make install; cd -

.PHONY: uninstall
uninstall:
	cd utils; make uninstall; cd -
	cd database_leveldb; make uninstall; cd -
	cd lvdbstorage; make uninstall; cd -
	cd queryeval; make uninstall; cd -
	cd queryproc; make uninstall; cd -

