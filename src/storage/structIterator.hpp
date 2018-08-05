/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STRUCTURE_BLOCK_ITERATOR_HPP_INCLUDED
#define _STRUS_STRUCTURE_BLOCK_ITERATOR_HPP_INCLUDED
#include "structBlock.hpp"
#include "documentBlockIteratorTemplate.hpp"
#include "databaseAdapter.hpp"

namespace strus {

/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class StorageClient;

class StructIterator
	:public DocumentBlockIteratorTemplate<DatabaseAdapter_StructBlock::Cursor,StructBlock>
{
public:
	StructIterator( const StorageClient* storage_, const DatabaseClientInterface* database_, Index structno_)
		:DocumentBlockIteratorTemplate<DatabaseAdapter_StructBlock::Cursor,StructBlock>( DatabaseAdapter_StructBlock::Cursor(database_,structno_))
		,m_storage(storage_)
		,m_structureScanner()
		,m_memberScanner()
		,m_structno(structno_){}
	~StructIterator(){}

	Index skipDoc( const Index& docno_)
	{
		if (docno() && docno_ == docno()) return docno_;
		m_structureScanner.clear();
		m_memberScanner.clear();
		return DocumentBlockIteratorTemplate<DatabaseAdapter_StructBlock::Cursor,StructBlock>::skipDoc( docno_);
	}

	IndexRange skipPosSource( const Index& firstpos_);
	IndexRange skipPosSink( const Index& firstpos_);

	IndexRange source() const
	{
		return m_structureScanner.initialized()
			? IndexRange( m_structureScanner.current()->header_start, m_structureScanner.current()->header_end)
			: IndexRange(0,0);
	}

	bool isCloseCandidate( const Index& docno_) const	{return docno_start() <= docno_ && docno_end() >= docno_;}

private:
	const StorageClient* m_storage;
	StructBlock::StructureScanner m_structureScanner;
	StructBlock::MemberScanner m_memberScanner;
	Index m_structno;
};

}//namespace
#endif

