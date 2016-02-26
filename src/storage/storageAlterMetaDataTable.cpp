/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#include "storageAlterMetaDataTable.hpp"
#include "storage.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "strus/arithmeticVariant.hpp"
#include "private/internationalization.hpp"
#include "private/utils.hpp"
#include "private/errorUtils.hpp"
#include <vector>
#include <string>

using namespace strus;

StorageAlterMetaDataTable::StorageAlterMetaDataTable(
		DatabaseClientInterface* database_,
		ErrorBufferInterface* errorhnd_)
	:m_database(database_)
	,m_commit(false)
	,m_rollback(false)
	,m_errorhnd(errorhnd_)
{
	m_metadescr_old.load( m_database.get());
	m_metadescr_new  = m_metadescr_old;
}

StorageAlterMetaDataTable::~StorageAlterMetaDataTable()
{
	if (!m_rollback && !m_commit) rollback();
}

bool StorageAlterMetaDataTable::commit()
{
	if (m_errorhnd->hasError())
	{
		m_errorhnd->explain( _TXT( "error in storage alter meta data table commit: %s"));
		return false;
	}
	if (m_commit)
	{
		m_errorhnd->report( _TXT( "called alter meta data table commit twice"));
		return false;
	}
	if (m_rollback)
	{
		m_errorhnd->report( _TXT( "called alter meta data table commit after rollback"));
		return false;
	}
	std::auto_ptr<DatabaseTransactionInterface>
		transaction( m_database->createTransaction());
	if (!transaction.get()) return false;
	try
	{
		MetaDataDescription::TranslationMap 
			trmap = m_metadescr_new.getTranslationMap(
					m_metadescr_old, m_metadescr_resets);
	
		MetaDataMap blockmap( m_database.get(), &m_metadescr_old);
		
		blockmap.rewriteMetaData( trmap, m_metadescr_new, transaction.get());
		m_metadescr_new.store( transaction.get());
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error in commit alter meta data transaction: %s"), *m_errorhnd, false);

	if (!transaction->commit()) return false;
	return m_commit = true;
}

void StorageAlterMetaDataTable::rollback()
{
	if (m_rollback)
	{
		m_errorhnd->report( _TXT( "called alter meta data table rollback twice"));
	}
	if (m_commit)
	{
		m_errorhnd->report( _TXT( "called alter meta data table rollback after commit"));
	}
	m_rollback = true;
}

void StorageAlterMetaDataTable::renameElementReset(
		const std::string& oldname,
		const std::string& name)
{
	std::vector<std::string>::iterator di = m_metadescr_resets.begin(), de = m_metadescr_resets.end();
	for (; di != de; ++di)
	{
		if (utils::caseInsensitiveEquals( oldname, *di))
		{
			*di = utils::tolower( name);
			break;
		}
	}
}

void StorageAlterMetaDataTable::changeElementType(
		const std::string& name,
		MetaDataElement::Type type)
{
	MetaDataDescription chgdescr;
	MetaDataDescription::const_iterator mi = m_metadescr_new.begin(), me = m_metadescr_new.end();
	for (; mi != me; ++mi)
	{
		if (utils::caseInsensitiveEquals( mi.name(), name))
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


void StorageAlterMetaDataTable::alterElement(
		const std::string& oldname,
		const std::string& name,
		const std::string& datatype)
{
	try
	{
		MetaDataElement::Type type = MetaDataElement::typeFromName( datatype.c_str());
	
		m_metadescr_old.renameElement( oldname, name);
		m_metadescr_new.renameElement( oldname, name);
		renameElementReset( oldname, name);
	
		(void)m_metadescr_new.getHandle( name); //... check if element exists
	
		changeElementType( name, type);
	}
	CATCH_ERROR_MAP( _TXT("error altering meta data element: %s"), *m_errorhnd);
}

void StorageAlterMetaDataTable::renameElement(
		const std::string& oldname,
		const std::string& name)
{
	try
	{
		m_metadescr_old.renameElement( oldname, name);
		m_metadescr_new.renameElement( oldname, name);
		renameElementReset( oldname, name);
	
		(void)m_metadescr_new.getHandle( name); //... check if element exists
	}
	CATCH_ERROR_MAP( _TXT("error renaming meta data element: %s"), *m_errorhnd);
}

void StorageAlterMetaDataTable::deleteElement(
		const std::string& name)
{
	try
	{
		(void)m_metadescr_new.getHandle( name); //... check if element exists
	
		MetaDataDescription chgdescr;
		MetaDataDescription::const_iterator mi = m_metadescr_new.begin(), me = m_metadescr_new.end();
		for (; mi != me; ++mi)
		{
			if (utils::caseInsensitiveEquals( mi.name(), name))
			{
				continue;
			}
			else
			{
				chgdescr.add( mi.element().type(), mi.name());
			}
		}
		m_metadescr_new = chgdescr;
		m_metadescr_resets.push_back( utils::tolower( name));
	}
	CATCH_ERROR_MAP( _TXT("error deleting meta data element: %s"), *m_errorhnd);
}

void StorageAlterMetaDataTable::clearElement(
		const std::string& name)
{
	try
	{
		m_metadescr_resets.push_back( utils::tolower( name));
	}
	CATCH_ERROR_MAP( _TXT("error clearing meta data element value: %s"), *m_errorhnd);
}

void StorageAlterMetaDataTable::addElement(
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

