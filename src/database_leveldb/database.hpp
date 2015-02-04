/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#ifndef _STRUS_DATABASE_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_DATABASE_IMPLEMENTATION_HPP_INCLUDED
#include "strus/databaseInterface.hpp"
#include <boost/thread/mutex.hpp>
#include <leveldb/db.h>

namespace strus
{

/// \brief Implementation of the strus key value storage database based on the LevelDB library
class Database
	:public DatabaseInterface
{
public:
	/// \param[in] path of the storage
	/// \param[in] cachesize_k number of K LRU cache for nodes
	/// \param[in] compression wheter to use snappy compression (true) or not
	Database( const char* path_, unsigned int cachesize_k, bool compression);

	virtual ~Database();

	virtual void close();

	virtual DatabaseTransactionInterface* createTransaction();

	virtual DatabaseCursorInterface* createCursor( bool useCache) const;

	virtual DatabaseBackupCursorInterface* createBackupCursor() const;
	
	virtual void writeImm(
			const char* key,
			std::size_t keysize,
			const char* value,
			std::size_t valuesize);

	virtual void removeImm(
			const char* key,
			std::size_t keysize);

	virtual bool readValue(
			const char* key,
			std::size_t keysize,
			std::string& value,
			bool useCache=false) const;

public:
	void cleanup();
	friend class DatabaseTransaction;

private:
	leveldb::DB* m_db;					///< levelDB handle
	leveldb::Options m_dboptions;				///< options for levelDB
	bool m_closed;						///< true, if 'close()' has been called
};

}//namespace
#endif


