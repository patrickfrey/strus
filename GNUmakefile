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
	@-mkdir -p $(INSTALL_INCDIR)/lib
	@cp include/strus/private/*.hpp $(INSTALL_INCDIR)/private/
	@cp include/strus/lib/*.hpp $(INSTALL_INCDIR)/lib/
	@cp include/strus/*.hpp $(INSTALL_INCDIR)

uninstall:
	cd src; make uninstall; cd ..
	@-rm -f $(INSTALL_INCDIR)/lib/storage.hpp
	@-rm -f $(INSTALL_INCDIR)/lib/database_leveldb.hpp
	@-rm -f $(INSTALL_INCDIR)/lib/queryeval.hpp
	@-rm -f $(INSTALL_INCDIR)/lib/queryproc.hpp
	@-rm -f $(INSTALL_INCDIR)/private/arithmeticVariantAsString.hpp
	@-rm -f $(INSTALL_INCDIR)/private/cmdLineOpt.hpp
	@-rm -f $(INSTALL_INCDIR)/private/fileio.hpp
	@-rm -f $(INSTALL_INCDIR)/private/configParser.hpp
	@-rm -f $(INSTALL_INCDIR)/databaseInterface.hpp
	@-rm -f $(INSTALL_INCDIR)/databaseCursorInterface.hpp
	@-rm -f $(INSTALL_INCDIR)/databaseTransactionInterface.hpp
	@-rm -f $(INSTALL_INCDIR)/databaseBackupCursorInterface.hpp
	@-rm -f $(INSTALL_INCDIR)/storagePeerInterface.hpp
	@-rm -f $(INSTALL_INCDIR)/storagePeerTransactionInterface.hpp
	@-rm -f $(INSTALL_INCDIR)/attributeReaderInterface.hpp
	@-rm -f $(INSTALL_INCDIR)/arithmeticVariant.hpp
	@-rm -f $(INSTALL_INCDIR)/constants.hpp
	@-rm -f $(INSTALL_INCDIR)/index.hpp
	@-rm -f $(INSTALL_INCDIR)/resultDocument.hpp
	@-rm -f $(INSTALL_INCDIR)/docnoIteratorInterface.hpp
	@-rm -f $(INSTALL_INCDIR)/forwardIteratorInterface.hpp
	@-rm -f $(INSTALL_INCDIR)/metaDataReaderInterface.hpp
	@-rm -f $(INSTALL_INCDIR)/postingIteratorInterface.hpp
	@-rm -f $(INSTALL_INCDIR)/postingJoinOperatorInterface.hpp
	@-rm -f $(INSTALL_INCDIR)/queryEvalInterface.hpp
	@-rm -f $(INSTALL_INCDIR)/queryInterface.hpp
	@-rm -f $(INSTALL_INCDIR)/queryProcessorInterface.hpp
	@-rm -f $(INSTALL_INCDIR)/statCounterValue.hpp
	@-rm -f $(INSTALL_INCDIR)/storageAlterMetaDataTableInterface.hpp
	@-rm -f $(INSTALL_INCDIR)/storageDocumentInterface.hpp
	@-rm -f $(INSTALL_INCDIR)/storageInterface.hpp
	@-rm -f $(INSTALL_INCDIR)/storageTransactionInterface.hpp
	@-rm -f $(INSTALL_INCDIR)/summarizerClosureInterface.hpp
	@-rm -f $(INSTALL_INCDIR)/summarizerFunctionInterface.hpp
	@-rm -f $(INSTALL_INCDIR)/weightedDocument.hpp
	@-rm -f $(INSTALL_INCDIR)/weightingClosureInterface.hpp
	@-rm -f $(INSTALL_INCDIR)/weightingFunctionInterface.hpp
	@-rm -f $(INSTALL_INCDIR)/invAclIteratorInterface.hpp
	@-rm -f $(INSTALL_INCDIR)/peerStorageTransactionInterface.hpp
	@-rm -f $(INSTALL_INCDIR)/peerStorageInterface.hpp
	@-rm -f $(INSTALL_INCDIR)/reference.hpp
	@-rmdir $(INSTALL_INCDIR)/private
	@-rmdir $(INSTALL_INCDIR)
	@-rmdir $(INSTALL_LIBDIR)

