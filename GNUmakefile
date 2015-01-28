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
	@-mkdir -p $(INSTALL_INCDIR)/private
	@cp include/strus/private/*.hpp $(INSTALL_INCDIR)/private/
	@cp include/strus/*.hpp $(INSTALL_INCDIR)

uninstall:
	cd src; make uninstall; cd ..
	@-rm $(INSTALL_INCDIR)/arithmeticVariant.hpp
	@-rm $(INSTALL_INCDIR)/private/arithmeticVariantAsString.hpp
	@-rm $(INSTALL_INCDIR)/private/cmdLineOpt.hpp
	@-rm $(INSTALL_INCDIR)/private/fileio.hpp
	@-rm $(INSTALL_INCDIR)/attributeReaderInterface.hpp
	@-rm $(INSTALL_INCDIR)/constants.hpp
	@-rm $(INSTALL_INCDIR)/docnoIteratorInterface.hpp
	@-rm $(INSTALL_INCDIR)/forwardIteratorInterface.hpp
	@-rm $(INSTALL_INCDIR)/index.hpp
	@-rm $(INSTALL_INCDIR)/metaDataReaderInterface.hpp
	@-rm $(INSTALL_INCDIR)/postingIteratorInterface.hpp
	@-rm $(INSTALL_INCDIR)/postingJoinOperatorInterface.hpp
	@-rm $(INSTALL_INCDIR)/queryEvalInterface.hpp
	@-rm $(INSTALL_INCDIR)/queryEvalLib.hpp
	@-rm $(INSTALL_INCDIR)/queryInterface.hpp
	@-rm $(INSTALL_INCDIR)/queryProcessorInterface.hpp
	@-rm $(INSTALL_INCDIR)/queryProcessorLib.hpp
	@-rm $(INSTALL_INCDIR)/resultDocument.hpp
	@-rm $(INSTALL_INCDIR)/statCounterValue.hpp
	@-rm $(INSTALL_INCDIR)/storageAlterMetaDataTableInterface.hpp
	@-rm $(INSTALL_INCDIR)/storageDocumentInterface.hpp
	@-rm $(INSTALL_INCDIR)/storageInterface.hpp
	@-rm $(INSTALL_INCDIR)/storageLib.hpp
	@-rm $(INSTALL_INCDIR)/storageTransactionInterface.hpp
	@-rm $(INSTALL_INCDIR)/summarizerClosureInterface.hpp
	@-rm $(INSTALL_INCDIR)/summarizerFunctionInterface.hpp
	@-rm $(INSTALL_INCDIR)/weightedDocument.hpp
	@-rm $(INSTALL_INCDIR)/weightingClosureInterface.hpp
	@-rm $(INSTALL_INCDIR)/weightingFunctionInterface.hpp
	@-rm $(INSTALL_INCDIR)/storage.hpp
	@-rm $(INSTALL_INCDIR)/invAclIteratorInterface.hpp
	@-rm $(INSTALL_INCDIR)/peerStorageTransactionInterface.hpp
	@-rm $(INSTALL_INCDIR)/peerStorageInterface.hpp
	@-rm $(INSTALL_INCDIR)/reference.hpp
	@-rmdir $(INSTALL_INCDIR)/private
	@-rmdir $(INSTALL_INCDIR)
	@-rmdir $(INSTALL_BINDIR)
	@-rmdir $(INSTALL_LIBDIR)

