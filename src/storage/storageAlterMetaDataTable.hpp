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
#ifndef _STRUS_LVDB_TRANSACTION_HPP_INCLUDED
#define _STRUS_LVDB_TRANSACTION_HPP_INCLUDED
#include "strus/storageAlterMetaDataTableInterface.hpp"
#include "strus/reference.hpp"
#include "metaDataBlock.hpp"
#include "metaDataRecord.hpp"
#include "metaDataMap.hpp"
#include "metaDataReader.hpp"
#include <vector>
#include <string>
#include <set>
#include <map>

namespace strus {
/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \class StorageTransaction
class StorageAlterMetaDataTable
	:public StorageAlterMetaDataTableInterface
{
private:
	StorageAlterMetaDataTable( const StorageAlterMetaDataTable&){}	//... non copyable
	void operator=( const StorageAlterMetaDataTable&){}		//... non copyable

public:
	StorageAlterMetaDataTable( DatabaseClientInterface* database_, ErrorBufferInterface* errorhnd_);

	~StorageAlterMetaDataTable();

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
	Reference<DatabaseClientInterface> m_database;		///< database interface

	MetaDataDescription m_metadescr_old;			///< meta data structure changes
	MetaDataDescription m_metadescr_new;			///< meta data structure changes
	std::vector<std::string> m_metadescr_resets;		///< meta data structure elements resets

	bool m_commit;						///< true, if the transaction has been committed
	bool m_rollback;					///< true, if the transaction has been rolled back
	ErrorBufferInterface* m_errorhnd;			///< buffer for reporting errors
};

}//namespace
#endif

