/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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

	/// \brief Move cursor to the least upper bound key stored in the database with upper limit specified by key and not by the domain key size
	/// \param[in] key pointer to the key of the item to seek
	/// \param[in] keysize size of 'key' in bytes
	/// \param[in] upkey pointer to the upper limit key that does not belong anymore to the key domain scanned
	/// \param[in] upkeysize size of 'upkey' in bytes
	/// \return the key of the element found or an undefined key
	virtual Slice seekUpperBoundRestricted(
			const char* key,
			std::size_t keysize,
			const char* upkey,
			std::size_t upkeysize)=0;

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


