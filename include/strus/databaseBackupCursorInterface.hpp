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
/// \brief Cursor interface to read the content of a snapshot of the key/value store database for backup
/// \file "databaseBackupCursorInterface.hpp"
#ifndef _STRUS_STORAGE_DATABASE_BACKUP_CURSOR_INTERFACE_HPP_INCLUDED
#define _STRUS_STORAGE_DATABASE_BACKUP_CURSOR_INTERFACE_HPP_INCLUDED
#include <string>

namespace strus
{

/// \brief Database cursor interface that can be used for backup
class DatabaseBackupCursorInterface
{
public:
	/// \brief Destructor
	virtual ~DatabaseBackupCursorInterface(){}

	/// \brief Fetches then next block to backup
	/// \param[out] key pointer to the key of the block
	/// \param[out] keysize size of key in bytes
	/// \param[out] blk pointer to the value of the block
	/// \param[out] blksize size of blk in bytes
	/// \return true on success, false, if there is no block left or an error occurred
	virtual bool fetch(
			const char*& key,
			std::size_t& keysize,
			const char*& blk,
			std::size_t& blksize)=0;
};
}//namespace
#endif

