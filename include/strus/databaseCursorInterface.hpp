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
/// \brief Cursor interface to iterate on elements of the key/value store database for retrieval
/// \file "databaseCursorInterface.hpp"
#ifndef _STRUS_DATABASE_CURSOR_INTERFACE_HPP_INCLUDED
#define _STRUS_DATABASE_CURSOR_INTERFACE_HPP_INCLUDED
#include <string>

namespace strus
{

/// \brief Interface to database cursor
class DatabaseCursorInterface
{
public:
	/// \brief Chunk data structure without ownership for keys and values of the database cursor
	class Slice
	{
	public:
		/// \brief Defaul constructor
		Slice()
			:m_ptr(0),m_size(0){}
		/// \brief Constructor
		/// \param[in] ptr_ pointer to data chunk
		/// \param[in] size_ size of ptr_ in bytes
		Slice( const char* ptr_, std::size_t size_)
			:m_ptr(ptr_),m_size(size_){}
		/// \brief Copy constructor
		/// \param[in] o chunk reference to copy
		Slice( const Slice& o)
			:m_ptr(o.m_ptr),m_size(o.m_size){}

		/// \brief Get the pointer to data
		/// \return the pointer
		const char* ptr() const		{return m_ptr;}
		/// \brief Get the size of the data in bytes
		/// \return the size
		std::size_t size() const	{return m_size;}

		/// \brief Cast of the data to a string
		operator std::string() const	{return std::string(m_ptr,m_size);}
		/// \brief Evaluate if the chunk is defined or a NULL reference
		/// \return true, if yes
		bool defined() const		{return m_ptr!=0;}

	private:
		const char* m_ptr;		///< pointer to data (no ownership)
		std::size_t m_size;		///< size of the data in bytes
	};

public:
	/// \brief Destructor
	virtual ~DatabaseCursorInterface(){}

	/// \brief Move cursor to the least upper bound key stored in the database
	/// \param[in] key pointer to the key of the item to seek
	/// \param[in] keysize size of 'key' in bytes
	/// \param[in] domainkeysize size of 'key' that defines the key value domain (the subset set of keys to search in)
	/// \return the key of the element found or an undefined key
	virtual Slice seekUpperBound(
			const char* key,
			std::size_t keysize,
			std::size_t domainkeysize)=0;

	/// \brief Move cursor to the first key stored in the database in a defined key domain
	/// \param[in] domainkey defines the key value domain (the subset set of keys to scan)
	/// \param[in] domainkeysize size of 'domainkey' in bytes
	/// \return the key of the element found or an undefined key
	virtual Slice seekFirst(
			const char* domainkey,
			std::size_t domainkeysize)=0;

	/// \brief Move cursor to the last key stored in the database in a defined key domain
	/// \param[in] domainkey defines the key value domain (the subset set of keys to scan)
	/// \param[in] domainkeysize size of 'domainkey' in bytes
	/// \return the key of the element found or an undefined key
	virtual Slice seekLast(
			const char* domainkey,
			std::size_t domainkeysize)=0;

	/// \brief Move cursor to the next key stored in the database in the current key domain
	/// \return the key of the element found or an undefined key
	virtual Slice seekNext()=0;

	/// \brief Move cursor to the previous key stored in the database in the current key domain
	/// \return the key of the element found or an undefined key
	virtual Slice seekPrev()=0;

	/// \brief Get the key of the current element
	/// \return the key of the current element or an undefined key
	virtual Slice key() const=0;

	/// \brief Get the value of the current element
	/// \return the value of the current element or an undefined key
	virtual Slice value() const=0;
};

}//namespace
#endif


