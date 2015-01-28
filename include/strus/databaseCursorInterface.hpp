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
#ifndef _STRUS_DATABASE_CURSOR_INTERFACE_HPP_INCLUDED
#define _STRUS_DATABASE_CURSOR_INTERFACE_HPP_INCLUDED
#include <string>

namespace strus
{

/// \brief Interface to database cursor
class DatabaseCursorInterface
{
public:
	/// \brief Destructor
	virtual ~DatabaseCursorInterface(){}

	/// \brief Move cursor to the least upper bound key stored in the database
	/// \param[in] key pointer to the key of the item to seek
	/// \param[in] keysize size of 'key' in bytes
	/// \param[in] domainkeysize size of 'key' that defines the key value domain (the subset set of keys to search in)
	/// \return the key of the element found or NULL
	virtual const char* seekUpperBound(
			const char* key,
			std::size_t keysize,
			std::size_t domainkeysize)=0;

	/// \brief Move cursor to the first key stored in the database in a defined key domain
	/// \param[in] domainkey defines the key value domain (the subset set of keys to scan)
	/// \param[in] domainkeysize size of 'domainkey' in bytes
	/// \return the key of the element found or NULL
	virtual const char* seekFirst(
			const char* domainkey,
			std::size_t domainkeysize)=0;

	/// \brief Move cursor to the last key stored in the database in a defined key domain
	/// \param[in] domainkey defines the key value domain (the subset set of keys to scan)
	/// \param[in] domainkeysize size of 'domainkey' in bytes
	/// \return the key of the element found or NULL
	virtual const char* seekLast(
			const char* domainkey,
			std::size_t domainkeysize)=0;

	/// \brief Move cursor to the next key stored in the database in the current key domain
	/// \return the key of the element found or NULL
	virtual const char* seekNext()=0;

	/// \brief Move cursor to the previous key stored in the database in the current key domain
	/// \return the key of the element found or NULL
	virtual const char* seekPrev()=0;

	/// \brief Get the value of the current element
	/// \param[out] value pointer to the element value
	/// \param[out] valuesize size of 'value' in bytes
	virtual void getValue(
			const char*& value,
			std::size_t& valuesize) const=0;

	/// \brief Get the value of a key as exact match (random access)
	/// \param[in] key pointer to the key of the item to retrieve
	/// \param[in] keysize size of 'key' in bytes
	/// \param[out] value pointer to the element value returned
	/// \param[out] valuesize size of 'value' in bytes
	virtual bool getKeyValue(
			const char* key,
			std::size_t keysize,
			const char*& value,
			std::size_t& valuesize)=0;

};

}//namespace
#endif


