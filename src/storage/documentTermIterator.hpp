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
/// \brief Implementation of the iterator on term occurrencies in documents (intended for feature selection)
/// \file "DocumentTermIterator.hpp"
#ifndef _STRUS_DOCUMENT_TERM_OCCURRENCE_ITERATOR_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_DOCUMENT_TERM_OCCURRENCE_ITERATOR_IMPLEMENTATION_HPP_INCLUDED
#include "strus/documentTermIteratorInterface.hpp"
#include "databaseAdapter.hpp"
#include "invTermBlock.hpp"

namespace strus
{

/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class StorageClient;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \class DocumentTermIterator
/// \brief Implementation of DocumentTermIteratorInterface
class DocumentTermIterator
	:public DocumentTermIteratorInterface
{
public:
	DocumentTermIterator(
			const StorageClient* storage_,
			const DatabaseClientInterface* database_,
			const std::string& type_,
			ErrorBufferInterface* errorhnd_);
	virtual ~DocumentTermIterator(){}

	virtual Index skipDoc( const Index& docno_);

	virtual bool nextTerm( Term& value);

	virtual unsigned int termDocumentFrequency( const Index& termno) const;

	virtual std::string termValue( const Index& termno) const;

private:
	const StorageClient* m_storage;					///< storage interface
	const DatabaseClientInterface* m_database;			///< database interface
	Index m_typeno;							///< typeno scope of this iterator
	Index m_docno;							///< current document number
	DatabaseAdapter_InverseTerm::Cursor m_dbadapter_inv;		///< db adapter for reading blocks with term occurrencies
	DatabaseAdapter_TermValueInv::Reader m_dbadapter_termno;	///< db adapter for reading term value strings with termno as key
	InvTermBlock m_invblk;						///< current block (document) iterated
	char const* m_invblkitr;					///< iterator on term occurrencies
	ErrorBufferInterface* m_errorhnd;				///< error buffer for exception free interface
};

}//namespace
#endif


