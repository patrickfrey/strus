/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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

	/// \brief Close client connection and do some cleanup
	virtual void close()=0;

	/// \brief Get the interpreted configuration this database client was created with
	/// \return the configuration as string
	virtual std::string config() const=0;
};

}//namespace
#endif


