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
#include <string>
#include <vector>
#include "strus/index.hpp"
#include "strus/statCounterValue.hpp"

namespace strus
{

/// \brief Forward declaration
class PostingIteratorInterface;
/// \brief Forward declaration
class ForwardIteratorInterface;
/// \brief Forward declaration
class StorageInserterInterface;

/// \brief Interface of a strus IR storage
class StorageInterface
{
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
	virtual PostingIteratorInterface*
		createTermPostingIterator(
			const std::string& type,
			const std::string& value)=0;

	/// \brief Create a viewer to inspect the term stored values with the forward index of the storage
	/// \param[in] type type name of the term to be inspected
	/// \return the created viewer reference to be disposed with delete
	virtual ForwardIteratorInterface*
		createForwardIterator(
			const std::string& type)=0;

	/// \brief Get the number of documents inserted into the collection
	/// \return the number of documents
	virtual Index nofDocumentsInserted() const=0;

	/// \brief Get the highest document number used in the collection
	/// \return the document number
	virtual Index maxDocumentNumber() const=0;

	/// \brief Get the internal document number
	/// \param[in] docid document id of the document inserted
	virtual Index documentNumber( const std::string& docid) const=0;

	/// \brief Get an element of document metadata
	/// \param[in] docno document number
	/// \param[in] varname variable name identifying the metadata attribute
	/// \return the metadata element
	virtual float documentMetaData( Index docno, char varname) const=0;

	/// \brief Get a string attribute value assigned to a document
	/// \param[in] docno document number
	/// \param[in] varname variable name identifying the attribute to get
	/// \return the attribute value
	virtual std::string documentAttribute( Index docno, char varname) const=0;

	/// \brief Create an object for the declaration of one insert/update of a document
	/// \param[in] docid document identifier (URI)
	/// \return the created inserter reference to be disposed with delete
	virtual StorageInserterInterface* createInserter( const std::string& docid)=0;

	/// \brief Forces flushing of all contents inserted persistently to the repository
	virtual void flush()=0;

	/// \brief Get some statistics (counters) of the storage
	virtual std::vector<StatCounterValue> getStatistics() const=0;
};

}//namespace
#endif


