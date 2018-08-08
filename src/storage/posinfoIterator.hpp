/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_POSINFO_ITERATOR_HPP_INCLUDED
#define _STRUS_POSINFO_ITERATOR_HPP_INCLUDED
#include "posinfoBlock.hpp"
#include "documentBlockIteratorTemplate.hpp"
#include "databaseAdapter.hpp"

namespace strus {

/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class StorageClient;

class PosinfoIterator
	:public DocumentBlockIteratorTemplate<DatabaseAdapter_PosinfoBlock::Cursor,PosinfoBlock>
{
public:
	PosinfoIterator( const StorageClient* storage_, const DatabaseClientInterface* database_, Index termtypeno_, Index termvalueno_)
		:DocumentBlockIteratorTemplate<DatabaseAdapter_PosinfoBlock::Cursor,PosinfoBlock>( DatabaseAdapter_PosinfoBlock::Cursor(database_,termtypeno_,termvalueno_))
		,m_storage(storage_)
		,m_positionScanner()
		,m_termtypeno(termtypeno_)
		,m_termvalueno(termvalueno_)
		,m_documentFrequency(-1){}
	~PosinfoIterator(){}

	Index skipDoc( const Index& docno_)
	{
		if (docno() && docno_ == docno()) return docno_;
		m_positionScanner.clear();
		return DocumentBlockIteratorTemplate<DatabaseAdapter_PosinfoBlock::Cursor,PosinfoBlock>::skipDoc( docno_);
	}

	Index skipPos( const Index& firstpos_);

	Index posno() const					{return m_positionScanner.initialized()?m_positionScanner.curpos():0;}
	bool isCloseCandidate( const Index& docno_) const	{return docno_start() <= docno_ && docno_end() >= docno_;}

	Index documentFrequency() const;
	unsigned int frequency() const;

private:
	const StorageClient* m_storage;
	PosinfoBlock::PositionScanner m_positionScanner;
	Index m_termtypeno;
	Index m_termvalueno;
	mutable Index m_documentFrequency;
};

}
#endif

