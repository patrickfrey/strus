/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for a transaction on the key/value store database
/// \file "databaseTransactionInterface.hpp"
#ifndef _STRUS_DATABASE_TRANSACTION_INTERFACE_HPP_INCLUDED
#define _STRUS_DATABASE_TRANSACTION_INTERFACE_HPP_INCLUDED
#include <cstddef>

namespace strus
{
/// \brief Forward declaration
class DatabaseCursorInterface;
/// \brief Forward declaration
class DatabaseOptions;

/// \brief Interface for transactions for writing on the strus key value storage database
class DatabaseTransactionInterface
{
public:
	/// \brief Destructor
	/// \remark Expected to do an implicit rollback, if neither 'commit()' or 'rollback' was called
	virtual ~DatabaseTransactionInterface(){}

	/// \brief Get an interface for reading values in the context of this transaction
	/// \param[in] options some hints how to access the key/value elements or blocks (e.g. fill LRU cache of not)
	/// \return the created cursor interface to be disposed with delete by the caller
	virtual DatabaseCursorInterface* createCursor( const DatabaseOptions& options) const=0;

	/// \brief Define a key/value pair to be written to the database as part of this transaction
	/// \param[in] key pointer to the key to write
	/// \param[in] keysize size of 'key' in bytes
	/// \param[in] value pointer to the value to write
	/// \param[in] valuesize size of 'value' in bytes
	/// \note Will replace existing duplicate entry in the database (not issuing an error)
	virtual void write(
			const char* key,
			std::size_t keysize,
			const char* value,
			std::size_t valuesize)=0;

	/// \brief Define a key to be deleted in the database as part of this transaction
	/// \param[in] key pointer to the key to delete
	/// \param[in] keysize size of 'key' in bytes
	virtual void remove(
			const char* key,
			std::size_t keysize)=0;

	/// \brief Define a set of nodes defined by a key prefix to be deleted in the database as part of this transaction
	/// \param[in] domainkey pointer to the key prefix to delete
	/// \param[in] domainkeysize size of 'domainkey' in bytes
	virtual void removeSubTree(
			const char* domainkey,
			std::size_t domainkeysize)=0;

	/// \brief Executes all commands defined in the transaction or none if one operation fails
	virtual bool commit()=0;

	/// \brief Rollback of the transaction, no changes made
	virtual void rollback()=0;
};

}//namespace
#endif


