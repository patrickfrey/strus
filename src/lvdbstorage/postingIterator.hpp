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
#include "docnoIterator.hpp"
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

	virtual ~PostingIterator();

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

	virtual unsigned int frequency()
	{
		return m_docnoitr.frequency();
	}

	virtual Index documentFrequency();

	virtual Index docno() const
	{
		return m_docno;
	}

	virtual Index posno() const
	{
		return m_posno;
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
	Index extractMatchData();
	Index getNextTermDoc();
	Index getFirstTermDoc( const Index& docno);

private:
	DocnoIterator m_docnoitr;
	leveldb::DB* m_db;
	Index m_termtypeno;
	Index m_termvalueno;
	DatabaseKey m_key;
	std::size_t m_keysize;
	Index m_docno;
	Index m_lastdocno;
	Index m_documentFrequency;
	leveldb::Iterator* m_itr;
	Index m_posno;
	const char* m_positr;
	const char* m_posend;
	std::string m_featureid;
};

}
#endif

