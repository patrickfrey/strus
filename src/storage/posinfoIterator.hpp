/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_POSINFO_ITERATOR_HPP_INCLUDED
#define _STRUS_POSINFO_ITERATOR_HPP_INCLUDED
#include "strus/reference.hpp"
#include "posinfoBlock.hpp"
#include "databaseAdapter.hpp"

namespace strus {

/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class StorageClient;

class PosinfoIterator
{
public:
	PosinfoIterator( const StorageClient* storage_, const DatabaseClientInterface* database_, Index termtypeno_, Index termvalueno_);
	~PosinfoIterator(){}

	Index skipDoc( const Index& docno_);
	Index skipPos( const Index& firstpos_);

	Index docno() const					{return m_docno;}
	Index posno() const					{return m_positionScanner.initialized()?m_positionScanner.curpos():0;}

	bool isCloseCandidate( const Index& docno_) const	{return m_docno_start <= docno_ && m_docno_end >= docno_;}
	Index documentFrequency() const;
	unsigned int frequency() const;

private:
	bool loadBlock( const Index& elemno_);

private:
	const StorageClient* m_storage;
	DatabaseAdapter_PosinfoBlock::Cursor m_dbadapter;
	PosinfoBlock m_posinfoBlk;
	PosinfoBlock::Cursor m_posinfoCursor;
	PosinfoBlock::PositionScanner m_positionScanner;
	Index m_termtypeno;
	Index m_termvalueno;
	Index m_docno;
	Index m_docno_start;
	Index m_docno_end;
	mutable Index m_documentFrequency;
};

}
#endif

