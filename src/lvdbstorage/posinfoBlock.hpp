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
#ifndef _STRUS_LVDB_POSINFO_BLOCK_HPP_INCLUDED
#define _STRUS_LVDB_POSINFO_BLOCK_HPP_INCLUDED
#include "dataBlock.hpp"
#include "databaseKey.hpp"
#include <leveldb/db.h>
#include <stdint.h>
#include <vector>

namespace strus {

typedef std::vector<Index> PosinfoBlockElement;

/// \class PosinfoBlock
/// \brief Block of term occurrence positions
class PosinfoBlock
	:public DataBlock
{
public:
	enum {
		BlockType=DatabaseKey::PosinfoBlockPrefix,
		MaxBlockSize=1024
	};
	static DatabaseKey databaseKey( const Index& typeno, const Index& termno)
	{
		return DatabaseKey( (char)BlockType, typeno, termno);
	}

public:
	explicit PosinfoBlock()
		:DataBlock( (char)BlockType){}
	PosinfoBlock( const PosinfoBlock& o)
		:DataBlock(o){}
	PosinfoBlock( const Index& id_, const void* ptr_, std::size_t size_)
		:DataBlock( (char)BlockType, id_, ptr_, size_){}

	void setId( const Index& id_);
	Index docno_at( const char* ref) const;
	std::vector<Index> positions_at( const char* itr) const;
	bool empty_at( const char* itr) const;
	const char* end_at( const char* itr) const;

	const char* begin() const
	{
		return charptr();
	}
	const char* end() const
	{
		return charend();
	}

	const char* nextDoc( const char* ref) const;
	const char* prevDoc( const char* ref) const;

	const char* find( const Index& docno_, const char* lowerbound) const;
	const char* upper_bound( const Index& docno_, const char* lowerbound) const;

	bool full() const
	{
		return size() >= MaxBlockSize;
	}
	bool isThisBlockAddress( const Index& docno_) const
	{
		return (docno_ <= id() && docno_ > docno_at( begin()));
	}
	/// \brief Check if the address 'docno_', if it exists, is in the following block we can get with 'leveldb::Iterator::Next()' or not
	bool isFollowBlockAddress( const Index& docno_) const
	{
		return (docno_ > id() && docno_ < id() + (MaxBlockSize/3)/* 3 ~ one byte docno,one byte pos,one byte 0xFF*/);
	}

	void append( const Index& docno, const std::vector<Index>& pos);
	void appendPositionsBlock( const char* start, const char* end);

	static PosinfoBlock merge( const PosinfoBlock& newblk, const PosinfoBlock& oldblk);

	class PositionIterator
	{
	public:
		PositionIterator()
			:m_itr(0),m_next(0),m_end(0),m_curpos(0){}
		PositionIterator( const char* start_, const char* end_);

		PositionIterator( const PositionIterator& o)
			:m_itr(o.m_itr),m_next(o.m_next),m_end(o.m_end),m_curpos(o.m_curpos){}

		PositionIterator& operator++()				{skip(); return *this;}
		PositionIterator operator++(int)			{PositionIterator rt( m_itr, m_end); skip(); return rt;}

		const Index& operator*() const				{return m_curpos;}

		bool eof() const					{return m_itr==m_end;}

		bool initialized() const				{return !!m_itr;}

		void init( const char* start_, const char* end_);
		void clear()						{init(0,0);}

	private:
		void skip();

		char const* m_itr;
		char const* m_next;
		const char* m_end;
		Index m_curpos;
	};

	PositionIterator positionIterator_at( const char* itr) const;
};

}
#endif

