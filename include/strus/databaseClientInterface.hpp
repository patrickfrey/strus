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
/// \brief Client interface to reading and writing to the key/value store database
/// \file "databaseClientInterface.hpp"
#ifndef _STRUS_DATABASE_CLIENT_INTERFACE_HPP_INCLUDED
#define _STRUS_DATABASE_CLIENT_INTERFACE_HPP_INCLUDED
#include <string>

namespace strus
{

/// \brief Forward declaration
class DatabaseTransactionInterface;
/// \brief Forward declaration
class DatabaseCursorInterface;
/// \brief Forward declaration
class DatabaseBackupCursorInterface;
/// \brief Forward declaration
class DatabaseOptions;

/// \brief Interface for accessing the strus key value storage database
class DatabaseClientInterface
{
public:
	/// \brief Destructor
	/// \remark Should call call 'close()' but ignore errors there silently
	virtual ~DatabaseClientInterface(){}

	/// \brief Create a transaction object for the database
	/// \return the created transaction interface to be disposed with delete by the caller
	virtual DatabaseTransactionInterface* createTransaction()=0;

	/// \brief Create an object for reading values from and iterating on the key value store database
	/// \param[in] options options for the created cursor
	/// \return the created cursor interface to be disposed with delete by the caller
	virtual DatabaseCursorInterface* createCursor( const DatabaseOptions& options) const=0;

	/// \brief Creates an object for iterating on a snapshot of the database that can be used for backup
	/// \return the created cursor interface to be disposed with delete by the caller
	virtual DatabaseBackupCursorInterface* createBackupCursor() const=0;

	/// \brief Write a key value immediately (synchronized)
	/// \param[in] key pointer to the key of the item to write
	/// \param[in] keysize size of the key of the item to write in bytes
	/// \param[in] value pointer to value to write
	/// \param[in] valuesize size of value to write in bytes
	/// \note Replaces existing duplicate entry in the database (not issuing an error)
	virtual void writeImm(
			const char* key,
			std::size_t keysize,
			const char* value,
			std::size_t valuesize)=0;

	/// \brief Delete a key value stored immediately (synchronized)
	/// \param[in] key pointer to the key of the item to write
	/// \param[in] keysize size of the key of the item to write in bytes
	virtual void removeImm(
			const char* key,
			std::size_t keysize)=0;

	/// \brief Read a value by key
	/// \param[in] key pointer to the key of the item to fetch
	/// \param[in] keysize size of the key of the item to fetch in bytes
	/// \param[out] value the value as string
	/// \param[in] options options as hints for the database
	/// \return true, if it was found
	virtual bool readValue(
			const char* key,
			std::size_t keysize,
			std::string& value,
			const DatabaseOptions& options) const=0;
};

}//namespace
#endif


