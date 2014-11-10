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
	@mkdir -p $(INSTALL_INCDIR)/utils
	@cp include/strus/*.hpp $(INSTALL_INCDIR)
	@cp include/strus/utils/*.hpp $(INSTALL_INCDIR)/utils

uninstall:
	cd src; make uninstall; cd ..
	@-rm $(INSTALL_INCDIR)/constants.hpp
	@-rm $(INSTALL_INCDIR)/forwardIndexViewerInterface.hpp
	@-rm $(INSTALL_INCDIR)/index.hpp
	@-rm $(INSTALL_INCDIR)/iteratorInterface.hpp
	@-rm $(INSTALL_INCDIR)/metaDataReaderInterface.hpp
	@-rm $(INSTALL_INCDIR)/queryEvalInterface.hpp
	@-rm $(INSTALL_INCDIR)/queryEvalLib.hpp
	@-rm $(INSTALL_INCDIR)/queryProcessorInterface.hpp
	@-rm $(INSTALL_INCDIR)/queryProcessorLib.hpp
	@-rm $(INSTALL_INCDIR)/resultDocument.hpp
	@-rm $(INSTALL_INCDIR)/storageInterface.hpp
	@-rm $(INSTALL_INCDIR)/storageLib.hpp
	@-rm $(INSTALL_INCDIR)/strus.hpp
	@-rm $(INSTALL_INCDIR)/summarizerInterface.hpp
	@-rm $(INSTALL_INCDIR)/weightedDocument.hpp
	@-rm $(INSTALL_INCDIR)/weightingFunctionInterface.hpp
	@-rm $(INSTALL_INCDIR)/utils/fileio.hpp
	@-rm $(INSTALL_INCDIR)/utils/cmdLineOpt.hpp
	@-rmdir $(INSTALL_INCDIR)/utils
	@-rmdir $(INSTALL_INCDIR)
	@-rmdir $(INSTALL_LIBDIR)



