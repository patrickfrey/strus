/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_DUMP_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_STORAGE_DUMP_IMPLEMENTATION_HPP_INCLUDED
#include "strus/storageDumpInterface.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/databaseCursorInterface.hpp"
#include "strus/reference.hpp"
#include <string>

namespace strus {

/// \brief Forward declaration
class ErrorBufferInterface;

class StorageDump
	:public StorageDumpInterface
{
public:
	StorageDump( const DatabaseInterface* database_, const std::string& configsrc, const std::string& keyprefix, ErrorBufferInterface* errorhnd_);
	virtual ~StorageDump(){}

	virtual bool nextChunk( const char*& chunk, std::size_t& chunksize);

	/// \brief How many key/value pairs to return in one chunk
	enum {NofKeyValuePairsPerChunk=256};

private:
	const DatabaseClientInterface* m_database;
	Reference<DatabaseCursorInterface> m_cursor;
	DatabaseCursorInterface::Slice m_key;
	std::string m_chunk;
	ErrorBufferInterface* m_errorhnd;			///< error buffer for exception free interface
};

}//namespace
#endif


