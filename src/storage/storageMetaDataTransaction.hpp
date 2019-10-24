/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_METADATA_TRANSACTION_HPP_INCLUDED
#define _STRUS_STORAGE_METADATA_TRANSACTION_HPP_INCLUDED
#include "strus/storageMetaDataTransactionInterface.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/reference.hpp"
#include "metaDataBlock.hpp"
#include "metaDataRecord.hpp"
#include "metaDataMap.hpp"
#include "metaDataReader.hpp"
#include "storageClient.hpp"
#include <vector>
#include <string>
#include <set>
#include <map>

namespace strus {
/// \brief Forward declaration
class ErrorBufferInterface;

/// \class StorageTransaction
class StorageMetaDataTransaction
	:public StorageMetaDataTransactionInterface
{
private:
	StorageMetaDataTransaction( const StorageMetaDataTransaction&){}	//... non copyable
	void operator=( const StorageMetaDataTransaction&){}		//... non copyable

public:
	StorageMetaDataTransaction(
			StorageClient* storage_,
			ErrorBufferInterface* errorhnd_);
	StorageMetaDataTransaction(
			StorageClient* storage_,
			ErrorBufferInterface* errorhnd_,
			const MetaDataDescription& metadescr_new);

	~StorageMetaDataTransaction();

	virtual void alterElement(
			const std::string& oldname,
			const std::string& name,
			const std::string& datatype);

	virtual void addElement(
			const std::string& name,
			const std::string& datatype);

	virtual void renameElement(
			const std::string& oldname,
			const std::string& name);

	virtual void deleteElement(
			const std::string& name);

	void clearElement(
			const std::string& name);

	/// \brief Transaction commit
	virtual bool commit();
	/// \brief Transaction rollback (automatically called with the destructor)
	virtual void rollback();

private:
	void renameElementReset(
			const std::string& oldname,
			const std::string& name);

	void changeElementType(
			const std::string& name,
			MetaDataElement::Type type);

private:
	StorageClient* m_storage;				///< storage to call refresh after commit or rollback

	MetaDataDescription m_metadescr_old;			///< meta data structure changes
	MetaDataDescription m_metadescr_new;			///< meta data structure changes
	std::vector<std::string> m_metadescr_resets;		///< meta data structure elements resets

	bool m_commit;						///< true, if the transaction has been committed
	bool m_rollback;					///< true, if the transaction has been rolled back
	ErrorBufferInterface* m_errorhnd;			///< buffer for reporting errors
};

}//namespace
#endif

