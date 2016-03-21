/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_FORWARD_INDEX_ITERATOR_HPP_INCLUDED
#define _STRUS_FORWARD_INDEX_ITERATOR_HPP_INCLUDED
#include "strus/forwardIteratorInterface.hpp"
#include "databaseAdapter.hpp"
#include "storageClient.hpp"
#include "forwardIndexBlock.hpp"
#include <string>

namespace strus
{

/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Forward index mapping document numbers to the document content
class ForwardIterator
	:public ForwardIteratorInterface
{
public:
	ForwardIterator(
		const StorageClient* storage_,
		const DatabaseClientInterface* database_,
		const std::string& type_,
		ErrorBufferInterface* errorhnd_);

	virtual ~ForwardIterator();

	/// \brief Define the document of the items inspected
	virtual void skipDoc( const Index& docno_);

	/// \brief Return the next matching position higher than or equal to firstpos in the current document.
	virtual Index skipPos( const Index& firstpos_);

	/// \brief Fetch the item at the current position
	virtual std::string fetch();

private:
	const DatabaseClientInterface* m_database;
	Reference<DatabaseAdapter_ForwardIndex::Cursor> m_dbadapter;
	ForwardIndexBlock m_curblock;
	Index m_curblock_firstpos;
	Index m_curblock_lastpos;
	char const* m_blockitr;
	Index m_docno;
	Index m_typeno;
	Index m_curpos;
	ErrorBufferInterface* m_errorhnd;			///< error buffer for exception free interface
};

}//namespace
#endif


