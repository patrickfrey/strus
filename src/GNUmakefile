TOPDIR=..

CPP_OBJS=

all: subdirs

.PHONY: subdirs
subdirs:
	cd lvdbstorage; make all; cd -
	cd queryeval; make all; cd -
	cd queryproc; make all; cd -
	cd strusCreate; make all; cd -

.PHONY: clean
clean:
	cd lvdbstorage; make clean; cd -
	cd queryeval; make clean; cd -
	cd queryproc; make clean; cd -
	cd strusCreate; make clean; cd -

.PHONY: install
install:
	cd lvdbstorage; make install; cd -
	cd queryeval; make install; cd -
	cd queryproc; make install; cd -
	cd strusCreate; make install; cd -

.PHONY: uninstall
uninstall:
	cd lvdbstorage; make uninstall; cd -
	cd queryeval; make uninstall; cd -
	cd queryproc; make uninstall; cd -
	cd strusCreate; make uninstall; cd -


