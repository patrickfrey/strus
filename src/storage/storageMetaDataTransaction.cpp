/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "storageMetaDataTransaction.hpp"
#include "storage.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "strus/base/local_ptr.hpp"
#include "strus/base/string_conv.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <vector>
#include <string>

using namespace strus;

StorageMetaDataTransaction::StorageMetaDataTransaction(
		StorageClient* storage_,
		ErrorBufferInterface* errorhnd_,
		const MetaDataDescription& metadescr_new_)
	:m_storage(storage_)
	,m_metadescr_old(),m_metadescr_new(metadescr_new_),m_metadescr_resets()
	,m_commit(false)
	,m_rollback(false)
	,m_errorhnd(errorhnd_)
{
	m_metadescr_old.load( m_storage->databaseClient());
}

StorageMetaDataTransaction::StorageMetaDataTransaction(
		StorageClient* storage_,
		ErrorBufferInterface* errorhnd_)
	:m_storage(storage_)
	,m_metadescr_old(),m_metadescr_new(),m_metadescr_resets()
	,m_commit(false)
	,m_rollback(false)
	,m_errorhnd(errorhnd_)
{
	m_metadescr_old.load( m_storage->databaseClient());
	m_metadescr_new  = m_metadescr_old;
}

StorageMetaDataTransaction::~StorageMetaDataTransaction()
{
	if (!m_rollback && !m_commit) rollback();
}

bool StorageMetaDataTransaction::commit()
{
	if (m_errorhnd->hasError())
	{
		m_errorhnd->explain( _TXT( "error in storage alter meta data table commit: %s"));
		return false;
	}
	if (m_commit)
	{
		m_errorhnd->report( ErrorCodeOperationOrder, _TXT( "called alter meta data table commit twice"));
		return false;
	}
	if (m_rollback)
	{
		m_errorhnd->report( ErrorCodeOperationOrder, _TXT( "called alter meta data table commit after rollback"));
		return false;
	}
	if (m_metadescr_resets.empty() && m_metadescr_old == m_metadescr_new)
	{
		// ... nothing has changed, do nothing, return with success
		return true;
	}
	StorageClient::TransactionLock lock( m_storage);

	strus::shared_ptr<MetaDataBlockCache> new_mdcache;
	strus::local_ptr<DatabaseTransactionInterface> transaction( m_storage->databaseClient()->createTransaction());
	if (!transaction.get()) return false;
	try
	{
		MetaDataDescription::TranslationMap trmap
			= m_metadescr_new.getTranslationMap( m_metadescr_old, m_metadescr_resets);

		MetaDataMap blockmap( m_storage->databaseClient(), m_storage->getMetaDataBlockCacheRef());
		blockmap.rewriteMetaData( trmap, m_metadescr_new, transaction.get());
		m_metadescr_new.store( transaction.get());
		new_mdcache.reset( new MetaDataBlockCache( m_storage->databaseClient(), m_metadescr_new));
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error in commit alter meta data transaction: %s"), *m_errorhnd, false);

	if (!transaction->commit()) return false;
	m_storage->resetMetaDataBlockCache( new_mdcache);
	return m_commit = true;
}

void StorageMetaDataTransaction::rollback()
{
	if (m_rollback)
	{
		m_errorhnd->report( ErrorCodeOperationOrder, _TXT( "called alter meta data table rollback twice"));
	}
	if (m_commit)
	{
		m_errorhnd->report( ErrorCodeOperationOrder, _TXT( "called alter meta data table rollback after commit"));
	}
	m_rollback = true;
}

void StorageMetaDataTransaction::renameElementReset(
		const std::string& oldname,
		const std::string& name)
{
	std::vector<std::string>::iterator di = m_metadescr_resets.begin(), de = m_metadescr_resets.end();
	for (; di != de; ++di)
	{
		if (strus::caseInsensitiveEquals( oldname, *di))
		{
			*di = string_conv::tolower( name);
			break;
		}
	}
}

void StorageMetaDataTransaction::changeElementType(
		const std::string& name,
		MetaDataElement::Type type)
{
	MetaDataDescription chgdescr;
	MetaDataDescription::const_iterator mi = m_metadescr_new.begin(), me = m_metadescr_new.end();
	for (; mi != me; ++mi)
	{
		if (strus::caseInsensitiveEquals( mi.name(), name))
		{
			chgdescr.add( type, name);
		}
		else
		{
			chgdescr.add( mi.element().type(), mi.name());
		}
	}
	m_metadescr_new = chgdescr;
}


void StorageMetaDataTransaction::alterElement(
		const std::string& oldname,
		const std::string& name,
		const std::string& datatype)
{
	try
	{
		MetaDataElement::Type type = MetaDataElement::typeFromName( datatype.c_str());
	
		if (m_metadescr_new.getHandle( oldname) < 0)
		{
			throw strus::runtime_error(_TXT("metadata element '%s' does not exist"), oldname.c_str());
		}
		m_metadescr_old.renameElement( oldname, name);
		m_metadescr_new.renameElement( oldname, name);
		renameElementReset( oldname, name);
	
		changeElementType( name, type);
	}
	CATCH_ERROR_MAP( _TXT("error altering meta data element: %s"), *m_errorhnd);
}

void StorageMetaDataTransaction::renameElement(
		const std::string& oldname,
		const std::string& name)
{
	try
	{
		if (m_metadescr_new.getHandle( oldname) < 0)
		{
			throw strus::runtime_error(_TXT("metadata element '%s' does not exist"), oldname.c_str());
		}
		m_metadescr_old.renameElement( oldname, name);
		m_metadescr_new.renameElement( oldname, name);
		renameElementReset( oldname, name);
	}
	CATCH_ERROR_MAP( _TXT("error renaming meta data element: %s"), *m_errorhnd);
}

void StorageMetaDataTransaction::deleteElement(
		const std::string& name)
{
	try
	{
		if (m_metadescr_new.getHandle( name) < 0)
		{
			throw strus::runtime_error(_TXT("deleted metadata element '%s' does not exist"), name.c_str());
		}
		MetaDataDescription chgdescr;
		MetaDataDescription::const_iterator mi = m_metadescr_new.begin(), me = m_metadescr_new.end();
		for (; mi != me; ++mi)
		{
			if (strus::caseInsensitiveEquals( mi.name(), name))
			{
				continue;
			}
			else
			{
				chgdescr.add( mi.element().type(), mi.name());
			}
		}
		m_metadescr_new = chgdescr;
		m_metadescr_resets.push_back( string_conv::tolower( name));
	}
	CATCH_ERROR_MAP( _TXT("error deleting meta data element: %s"), *m_errorhnd);
}

void StorageMetaDataTransaction::deleteElements()
{
	try
	{
		m_metadescr_new.clear();
	}
	CATCH_ERROR_MAP( _TXT("error deleting all meta data elements: %s"), *m_errorhnd);
}

void StorageMetaDataTransaction::clearElement(
		const std::string& name)
{
	try
	{
		m_metadescr_resets.push_back( string_conv::tolower( name));
	}
	CATCH_ERROR_MAP( _TXT("error clearing meta data element value: %s"), *m_errorhnd);
}

void StorageMetaDataTransaction::addElement(
		const std::string& name,
		const std::string& datatype)
{
	try
	{
		MetaDataElement::Type type = MetaDataElement::typeFromName( datatype.c_str());
		m_metadescr_new.add( type, name);
	}
	CATCH_ERROR_MAP( _TXT("error adding meta data element: %s"), *m_errorhnd);
}

