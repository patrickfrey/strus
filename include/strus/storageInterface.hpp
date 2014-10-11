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
#ifndef _STRUS_STORAGE_INTERFACE_HPP_INCLUDED
#define _STRUS_STORAGE_INTERFACE_HPP_INCLUDED
#include "strus/iteratorInterface.hpp"
#include <string>

namespace strus
{

class StorageInterface
{
public:
	class TransactionInterface
	{
	public:
		/// \brief Destructor that is doing the transaction rollback too
		virtual ~TransactionInterface(){}

		/// \brief Add one occurrence of a term, throws on std::bad_alloc
		/// \param[in] type_ type name of the term
		/// \param[in] value_ value string of the term
		/// \param[in] position_ position of the term in the document
		virtual void addTermOccurrence(
				const std::string& type_,
				const std::string& value_,
				const Index& position_)=0;

		/// \brief Define the weight of the term in the document (allows to define one single measure like tf*IDF for non positional weighting of terms)
		/// \param[in] type_ type name of the term
		/// \param[in] value_ value string of the term
		/// \param[in] weight_ weight of the term in the document
		virtual void setTermWeight(
				const std::string& type_,
				const std::string& value_,
				float weight_)=0;

		/// \brief Commit of the transaction, throws on error
		virtual void commit()=0;
	};

public:
	/// \brief Destructor
	/// \remark Calls close but ignores errors there silently
	virtual ~StorageInterface(){}

	/// \brief Close the storage and throw on error
	/// \remark Call this function before the destructor if you want to catch errors in the close
	virtual void close(){};

	/// \brief Create an iterator on the occurrencies of a term in the storage
	/// \param[in] type type name of the term
	/// \param[in] value value string of the term
	/// \return the created iterator reference to be disposed with delete
	virtual IteratorInterface*
		createTermOccurrenceIterator(
			const std::string& type,
			const std::string& value)=0;

	/// \brief Create an insert/update transaction for a document
	/// \param[in] docid Document identifier (URI)
	/// \return the created transaction reference to be disposed with delete
	virtual TransactionInterface* createTransaction( const std::string& docid)=0;
};

}//namespace
#endif


