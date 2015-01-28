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
#ifndef _STRUS_DATABASE_INTERFACE_HPP_INCLUDED
#define _STRUS_DATABASE_INTERFACE_HPP_INCLUDED
#include <string>

namespace strus
{

/// \brief Forward declaration
class DatabaseTransactionInterface;
/// \brief Forward declaration
class DatabaseCursorInterface;

/// \brief Interface for accessing the strus key value storage database
class DatabaseInterface
{
public:
	/// \brief Destructor
	/// \remark Should call call 'close()' but ignore errors there silently
	virtual ~DatabaseInterface(){}

	/// \brief Close the database client connection and throw on error
	/// \remark Call this function before the destructor if you want to catch errors in the close
	virtual void close()=0;

	/// \brief Create a transaction object for the database
	/// \return the created transaction interface to be disposed with delete by the caller
	virtual DatabaseTransactionInterface* createTransaction()=0;

	/// \brief Create an object for reading values from and iterating on the key value store database
	/// \param[in] useCache Hint for cursor to cache visited key/value elements or blocks
	/// \return the created cursor interface to be disposed with delete by the caller
	virtual DatabaseCursorInterface* createCursor( bool useCache)=0;

	/// \brief Write a key value immediately (synchronized)
	/// \param[in] key pointer to the key of the item to write
	/// \param[in] keysize size of the key of the item to write in bytes
	/// \param[in] val pointer to value to write
	/// \param[in] valsize size of value to write in bytes
	/// \note Replaces existing duplicate entry in the database (not issuing an error)
	virtual void writeImm(
			const char* key,
			std::size_t keysize,
			const char* value,
			std::size_t valuesize)=0;

	/// \brief Delete a key value stored immediately (synchronized)
	/// \param[in] key pointer to the key of the item to write
	/// \param[in] keysize size of the key of the item to write in bytes
	virtual void deleteImm(
			const char* key,
			std::size_t keysize)=0;

	/// \brief Read a value by key
	/// \param[in] key pointer to the key of the item to fetch
	/// \param[in] keysize size of the key of the item to fetch in bytes
	/// \param[out] value the value as string
	/// \return true, if it was found
	virtual bool readValue(
			const char* key,
			std::size_t keysize,
			std::string& value,
			bool useCache=false) const=0;
};

}//namespace
#endif


