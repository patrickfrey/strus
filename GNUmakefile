include variables.mk

all:
	cd src; make all; cd ..
	cd tests; make all; cd ..

test: all
	cd tests; make test; cd ..

clean:
	cd tests; make clean; cd ..
	cd src; make clean; cd ..

install:
	cd src; make install; cd ..
	@mkdir -p $(INSTALL_INCDIR)
	@cp include/strus/*.hpp $(INSTALL_INCDIR)

uninstall:
	cd src; make uninstall; cd ..
	@rmdir $(INSTALL_INCDIR)
	@rmdir $(INSTALL_LIBDIR)
	@rmdir $(INSTALL_BINDIR)



