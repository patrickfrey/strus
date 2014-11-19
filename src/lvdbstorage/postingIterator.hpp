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
#ifndef _STRUS_LVDB_ITERATOR_HPP_INCLUDED
#define _STRUS_LVDB_ITERATOR_HPP_INCLUDED
#include "strus/postingIteratorInterface.hpp"
#include "docnoBlock.hpp"
#include "posinfoBlock.hpp"
#include "blockStorage.hpp"
#include "databaseKey.hpp"
#include <leveldb/db.h>

namespace strus {
/// \brief Forward declaration
class MetaDataReader;

class PostingIterator
	:public PostingIteratorInterface
{
public:
	PostingIterator( leveldb::DB* db_, Index termtypeno, Index termvalueno, const char* termstr);
	PostingIterator( const PostingIterator& o);

	virtual ~PostingIterator(){}

	virtual std::vector<PostingIteratorInterface*> subExpressions( bool positive)
	{
		return std::vector<PostingIteratorInterface*>();
	}
	virtual const std::string& featureid() const
	{
		return m_featureid;
	}

	virtual Index skipDoc( const Index& docno_);
	virtual Index skipPos( const Index& firstpos_);

	virtual unsigned int frequency();

	virtual Index documentFrequency();

	virtual Index docno() const
	{
		return m_docno;
	}

	virtual Index posno() const
	{
		if (m_positionScanner.initialized())
		{
			return m_positionScanner.curpos();
		}
		return 0;
	}

	virtual float weight() const
	{
		return 0.0;
	}

	virtual PostingIteratorInterface* copy() const
	{
		return new PostingIterator(*this);
	}

private:
	Index skipDocPosinfoBlock( const Index& docno_);
	Index skipDocDocnoBlock( const Index& docno_);

private:
	leveldb::DB* m_db;

	BlockStorage<DocnoBlock> m_docnoStorage;
	const DocnoBlock* m_docnoBlk;
	DocnoBlock::const_iterator m_docnoItr;

	BlockStorage<PosinfoBlock> m_posinfoStorage;
	const PosinfoBlock* m_posinfoBlk;
	char const* m_posinfoItr;
	PosinfoBlock::PositionScanner m_positionScanner;

	Index m_last_docno;
	Index m_docno;
	Index m_termtypeno;
	Index m_termvalueno;
	Index m_documentFrequency;
	std::string m_featureid;
};

}
#endif

