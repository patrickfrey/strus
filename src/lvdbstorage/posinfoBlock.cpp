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
#include "posinfoBlock.hpp"
#include "indexPacker.hpp"
#include <cstring>
#include <string.h>

using namespace strus;

enum {EndPosinfoMarker=(char)0xFE};

Index PosinfoBlock::docno_at( const char* ref) const
{
	char const* rr = ref;
	if (rr == charend()) return 0;
	return id() - (unpackIndex( rr, charend()) - 1);
}

std::vector<Index> PosinfoBlock::positions_at( const char* itr) const
{
	std::vector<Index> rt;
	char const* pi = skipIndex( itr, charend());	//... skip docno
	const char* pe = charend();
	while (pi != pe && *pi != EndPosinfoMarker)
	{
		rt.push_back( unpackIndex( pi, pe));
	}
	return rt;
}

const char* PosinfoBlock::end_at( const char* itr) const
{
	char const* pi = skipIndex( itr, charend());	//... skip docno
	const char* pe = charend();
	while (pi != pe && *pi != EndPosinfoMarker)
	{
		++pi;
	}
	return pi;
}

bool PosinfoBlock::empty_at( const char* itr) const
{
	char const* pi = skipIndex( itr, charend());	//... skip docno
	return (pi != charend() || *pi == EndPosinfoMarker);
}

const char* PosinfoBlock::nextDoc( const char* ref) const
{
	if (ref == charend()) return 0;
	char const* rt = (const char*)std::memchr( ref, EndPosinfoMarker, charend()-ref);
	return (rt)?(rt+1):charend();
}

const char* PosinfoBlock::prevDoc( const char* ref) const
{
	if (ref == charptr()) return 0;
	char const* rt = (const char*)::memrchr( charptr(), EndPosinfoMarker, ref-charptr());
	return (rt)?(rt+1):charptr();
}

void PosinfoBlock::append( const Index& docno, const std::vector<Index>& pos)
{
	char const* pp = prevDoc( charend());
	if (pp && docno_at( pp) > docno)
	{
		throw std::runtime_error( "posinfo not added in ascending docno order");
	}
	if (id() < docno)
	{
		throw std::runtime_error( "internal: upper bound of docno in posinfo block not set (setId)");
	}
	std::string blk;
	if (size()) blk.push_back( EndPosinfoMarker);
	packIndex( blk, id() - docno + 1);
	std::vector<Index>::const_iterator pi = pos.begin(), pe = pos.end();
	for (; pi != pe; ++pi)
	{
		packIndex( blk, *pi);
	}
	DataBlock::append( blk.c_str(), blk.size());
}

void PosinfoBlock::appendPositionsBlock( const char* start, const char* end)
{
	char ch = EndPosinfoMarker;
	if (size()) DataBlock::append( (const void*)&ch, 1);
	DataBlock::append( (const void*)start, end-start);
}


PosinfoBlock::PositionIterator::PositionIterator( const char* start_, const char* end_)
	:m_itr(start_),m_next(start_),m_end(end_)
{
	m_curpos = (start_ == end_)?0:unpackIndex( m_next, m_end);
}

void PosinfoBlock::PositionIterator::skip()
{
	m_itr = m_next;
	m_curpos = (m_next == m_end)?0:unpackIndex( m_next, m_end);
}

void PosinfoBlock::PositionIterator::init( const char* start_, const char* end_)
{
	m_itr = start_;
	m_next = start_;
	m_end = end_;
	m_curpos = (start_ == end_)?0:unpackIndex( m_next, m_end);
}

PosinfoBlock::PositionIterator PosinfoBlock::positionIterator_at( const char* itr) const
{
	if (itr == charend()) return PositionIterator();
	char const* pi = skipIndex( itr, charend());	//... skip docno
	if (pi == charend()) return PositionIterator( charend(),charend());
	char const* pe = nextDoc(pi);
	return PositionIterator( pi, pe);
}

const char* PosinfoBlock::upper_bound( const Index& docno_, const char* lowerbound) const
{
	if (!lowerbound || lowerbound == charend()) return charend();
	char const* last = charend();
	char const* first = lowerbound;
	Index dn = docno_at( first);
	if (dn >= docno_) return first;

	unsigned int ofs = 128;
	while (last - first > ofs)
	{
		char const* fwdpos = nextDoc( lowerbound + (ofs>>1));
		if (!fwdpos)
		{
			ofs >>= 2;
			if (ofs < 16) break;
		}
		else if ((dn=docno_at(fwdpos)) >= docno_)
		{
			if (dn == docno_) return fwdpos;
			ofs >>= 2;
			if (ofs < 16) break;
		}
		else
		{
			first = fwdpos;
		}
	}
	while (first != last)
	{
		Index dn = docno_at( first);
		if (dn >= docno_) return first;
	}
	return 0;
}

const char* PosinfoBlock::find( const Index& docno_, const char* lowerbound) const
{
	const char* rt = upper_bound( docno_, lowerbound);
	return (docno_ == docno_at( rt))?rt:0;
}

void PosinfoBlock::setId( const Index& id_)
{
	char const* pp = prevDoc( charend());
	Index maxDocno = docno_at( pp);
	
	if (maxDocno > id_) throw std::runtime_error( "internal: cannot set posinfo block id to a smaller value than the highest docno inserted");
	if (id() != id_)
	{
		if (empty())
		{
			DataBlock::setId( id_);
		}
		else
		{
			// Rewrite document references (first element in variable size record):
			std::string content;
			Index id_diff = id_ - id();
			char const* bi = begin();
			const char* be = end();
			while (bi != be)
			{
				packIndex( content, unpackIndex( bi, be) + id_diff);
				while (bi != be && *bi != EndPosinfoMarker)
				{
					packIndex( content, unpackIndex( bi, be));
				}
				if (bi != be)
				{
					content.push_back( EndPosinfoMarker);
					++bi;
				}
			}
			init( id_, content.c_str(), content.size(), content.size());
		}
	}
}

PosinfoBlock PosinfoBlock::merge( const PosinfoBlock& newblk, const PosinfoBlock& oldblk)
{
	PosinfoBlock rt;
	char const* newi = newblk.begin();
	char const* oldi = oldblk.begin();
	Index newdn = newblk.docno_at( newi);
	Index olddn = newblk.docno_at( oldi);

	while (newdn && olddn)
	{
		if (newdn <= olddn)
		{
			if (!newblk.empty_at( newi))
			{
				//... append only if not empty (empty => delete)
				rt.appendPositionsBlock( newi, newblk.end_at( newi));
			}
			if (newdn == olddn)
			{
				//... defined twice -> prefer new entry and ignore old
				oldi = oldblk.nextDoc( oldi);
				olddn = newblk.docno_at( oldi);
			}
			newi = newblk.nextDoc( newi);
			newdn = newblk.docno_at( newi);
		}
		else
		{
			if (!oldblk.empty_at( oldi))
			{
				//... append only if not empty (empty => delete)
				rt.appendPositionsBlock( oldi, oldblk.end_at( oldi));
			}
			oldi = oldblk.nextDoc( oldi);
			olddn = newblk.docno_at( oldi);
		}
	}
	while (newdn)
	{
		if (!newblk.empty_at( newi))
		{
			//... append only if not empty (empty => delete)
			rt.appendPositionsBlock( newi, newblk.end_at( newi));
		}
		newi = newblk.nextDoc( newi);
		newdn = newblk.docno_at( newi);
	}
	while (olddn)
	{
		if (!oldblk.empty_at( oldi))
		{
			//... append only if not empty (empty => delete)
			rt.appendPositionsBlock( oldi, oldblk.end_at( oldi));
		}
		oldi = oldblk.nextDoc( oldi);
		olddn = newblk.docno_at( oldi);
	}
	return rt;
}

