/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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


