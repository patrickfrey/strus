/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Helper class to define an undefined database client for the case that the pointer to the database is assumed to exist
/// \file databaseClientUndefinedStub.hpp
#ifndef _STRUS_CORE_DATABASE_CLIENT_UNDEFINEFD_HPP_INCLUDED
#define _STRUS_CORE_DATABASE_CLIENT_UNDEFINEFD_HPP_INCLUDED
#include "private/internationalization.hpp"
#include "strus/errorBufferInterface.hpp"

/// \brief strus toplevel namespace
namespace strus
{

/// \brief Implementation of the strus key value storage database based on the LevelDB library
class DatabaseClientUndefinedStub
	:public DatabaseClientInterface
{
public:
	DatabaseClientUndefinedStub( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_)
	{}
	virtual ~DatabaseClientUndefinedStub(){}

	virtual DatabaseTransactionInterface* createTransaction()
		{NOT_IMPLEMENTED(); return NULL;}
	virtual DatabaseCursorInterface* createCursor( const DatabaseOptions& options) const
		{NOT_IMPLEMENTED(); return NULL;}
	virtual DatabaseBackupCursorInterface* createBackupCursor() const
		{NOT_IMPLEMENTED(); return NULL;}
	virtual void writeImm( const char* key, std::size_t keysize, const char* value, std::size_t valuesize)
		{NOT_IMPLEMENTED();}
	virtual void removeImm( const char* key, std::size_t keysize)
		{NOT_IMPLEMENTED();}
	virtual bool readValue( const char* key, std::size_t keysize, std::string& value, const DatabaseOptions& options) const
		{NOT_IMPLEMENTED(); return false;}
	virtual std::string config() const
		{NOT_IMPLEMENTED(); return std::string();}
	virtual bool compactDatabase()
		{NOT_IMPLEMENTED(); return false;}
	virtual void close()
		{NOT_IMPLEMENTED();}

private:
	void NOT_IMPLEMENTED() const
	{
		ErrorCode ec = ErrorCodeNotImplemented;
		m_errorhnd->report( ec, _TXT("database stub %s"), strus::errorCodeToString( ec));
	}
	mutable ErrorBufferInterface* m_errorhnd;			///< buffer for reporting errors
};

}//namespace
#endif

