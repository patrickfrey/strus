/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_INDEX_SET_ITERATOR_HPP_INCLUDED
#define _STRUS_STORAGE_INDEX_SET_ITERATOR_HPP_INCLUDED
#include "booleanBlock.hpp"
#include "databaseAdapter.hpp"

namespace strus {

/// \brief Forward declaration
class DatabaseClientInterface;

class IndexSetIterator
{
public:
	IndexSetIterator(
			const DatabaseClientInterface* database_,
			DatabaseKey::KeyPrefix dbprefix_,
			const BlockKey& key_,
			bool useCache_);
	~IndexSetIterator(){}

	Index skip( const Index& elemno_);
	Index elemno() const			{return m_elemno;}

private:
	bool loadBlock( const Index& elemno_);

private:
	DatabaseAdapter_BooleanBlock::Cursor m_dbadapter;
	BooleanBlock m_elemBlk;
	BooleanBlock::NodeCursor m_elemCursor;

	Index m_elemno;
};

}
#endif

