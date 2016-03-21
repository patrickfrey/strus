/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_DATABASE_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_DATABASE_IMPLEMENTATION_HPP_INCLUDED
#include "strus/databaseInterface.hpp"
#include "databaseClient.hpp"
#include <leveldb/db.h>

namespace strus {

/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class DatabaseBackupCursorInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Interface to the create,destroy the key value store database
class Database
	:public DatabaseInterface
{
public:
	explicit Database( ErrorBufferInterface* errorhnd_)
		:m_dbhandle_map( new LevelDbHandleMap()),m_errorhnd(errorhnd_){}

	virtual DatabaseClientInterface* createClient( const std::string& configsource) const;

	virtual bool exists( const std::string& configsource) const;

	virtual bool createDatabase( const std::string& configsource) const;

	virtual bool destroyDatabase( const std::string& configsource) const;

	virtual bool restoreDatabase( const std::string& configsource, DatabaseBackupCursorInterface* backup) const;

	virtual const char* getConfigDescription( ConfigType type) const;

	virtual const char** getConfigParameters( ConfigType type) const;

private:
	utils::SharedPtr<LevelDbHandleMap> m_dbhandle_map;
	ErrorBufferInterface* m_errorhnd;	///< buffer for reporting errors
};

}//namespace
#endif

