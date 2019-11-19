/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_FF_ITERATOR_HPP_INCLUDED
#define _STRUS_STORAGE_FF_ITERATOR_HPP_INCLUDED
#include "ffBlock.hpp"
#include "documentBlockIteratorTemplate.hpp"
#include "databaseAdapter.hpp"

namespace strus {

/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class StorageClient;

class FfIterator
	:public DocumentBlockIteratorTemplate<DatabaseAdapter_FfBlock::Cursor,FfBlock,FfIndexNodeCursor>
{
public:
	FfIterator( const StorageClient* storage_, const DatabaseClientInterface* database_, Index termtypeno_, Index termvalueno_)
		:DocumentBlockIteratorTemplate<DatabaseAdapter_FfBlock::Cursor,FfBlock,FfIndexNodeCursor>( DatabaseAdapter_FfBlock::Cursor(database_,termtypeno_,termvalueno_))
		,m_storage(storage_)
		,m_termtypeno(termtypeno_)
		,m_termvalueno(termvalueno_)
		,m_posno(0)
		,m_documentFrequency(-1){}
	~FfIterator(){}

	Index skipDoc( const Index& docno_)
	{
		if (docno() && docno_ == docno()) return docno_;
		m_posno = 0;
		return DocumentBlockIteratorTemplate<DatabaseAdapter_FfBlock::Cursor,FfBlock,FfIndexNodeCursor>::skipDoc( docno_);
	}

	Index skipPos( const Index& firstpos_)			{if (currentBlock().frequency_at( currentBlockCursor())) return m_posno = (firstpos_ > 1) ? 0 : 1; else return m_posno=0;}

	Index posno() const					{return m_posno;}
	bool isCloseCandidate( const Index& docno_) const	{return docno_start() <= docno_ && docno_end() >= docno_;}

	Index documentFrequency() const;
	unsigned int frequency() const				{return docno() ? currentBlock().frequency_at( currentBlockCursor()) : 0;}

private:
	const StorageClient* m_storage;
	Index m_termtypeno;
	Index m_termvalueno;
	Index m_posno;
	mutable Index m_documentFrequency;
};

}//namespace
#endif

