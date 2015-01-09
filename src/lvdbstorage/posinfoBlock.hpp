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
#include <vector>
#include <map>

namespace strus {

/// \class PosinfoBlock
/// \brief Block of term occurrence positions
class PosinfoBlock
	:public DataBlock
{
public:
	enum {
		MaxBlockSize=1024
	};

public:
	explicit PosinfoBlock()
		:DataBlock( (char)DatabaseKey::PosinfoBlockPrefix){}
	PosinfoBlock( const PosinfoBlock& o)
		:DataBlock(o){}
	PosinfoBlock( const Index& id_, const void* ptr_, std::size_t size_)
		:DataBlock( (char)DatabaseKey::PosinfoBlockPrefix, id_, ptr_, size_){}

	PosinfoBlock& operator=( const PosinfoBlock& o)
	{
		DataBlock::operator =(o);
		return *this;
	}

	void setId( const Index& id_);
	void swap( PosinfoBlock& o)
	{
		DataBlock::swap( o);
	}
	Index docno_at( const char* ref) const;
	std::vector<Index> positions_at( const char* itr) const;
	unsigned int frequency_at( const char* itr) const;
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

	const char* endOfDoc( const char* ref) const;
	const char* nextDoc( const char* ref) const;
	const char* prevDoc( const char* ref) const;

	Index relativeIndexFromDocno( const Index& docno_) const {return id()-docno_+1;}
	Index docnoFromRelativeIndex( const Index& dcidx_) const {return id()-dcidx_+1;}

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
	/// \brief Check if the address 'docno_', if it exists, probably is in the following block we can get with 'leveldb::Iterator::Next()' or not
	bool isFollowBlockAddress( const Index& docno_) const
	{
		Index diff = id() - docno_at( begin());
		return (docno_ > id()) && (docno_ < id() + diff - (diff>>4));
	}

	void append( const Index& docno, const std::vector<Index>& pos);
	void append( const Index& docno, const char* posinfo);
	void appendPositionsBlock( const char* start, const char* end);

	class PositionScanner
	{
	public:
		PositionScanner()
			:m_start(0),m_end(0),m_itr(0),m_curpos(0),m_lastpos(0){}
		PositionScanner( const char* start_, const char* end_)
			:m_start(start_),m_end(end_),m_itr(start_),m_curpos(0),m_lastpos(0){}
		PositionScanner( const PositionScanner& o)
			:m_start(o.m_start),m_end(o.m_end),m_itr(o.m_itr),m_curpos(o.m_curpos),m_lastpos(o.m_lastpos){}

		bool initialized() const				{return !!m_itr;}

		void init( const char* start_, const char* end_)
		{
			m_itr = start_;
			m_start = start_;
			m_end = end_;
			m_curpos = 0;
			m_lastpos = 0;
		}

		void clear()						{init(0,0);}

		Index curpos() const					{return m_curpos;}
		Index skip( const Index& pos);

	private:
		const char* m_start;
		const char* m_end;
		char const* m_itr;
		Index m_curpos;
		Index m_lastpos;
	};

	PositionScanner positionScanner_at( const char* itr) const;
};

}//namespace
#endif

