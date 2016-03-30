/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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

