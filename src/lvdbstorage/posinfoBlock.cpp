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
#include <limits>
#include <iostream>
#include <string.h>

using namespace strus;

enum {EndPosinfoMarker=(char)0xFE};

std::ostream& strus::operator<< (std::ostream& out, const PosinfoBlockElement& e)
{
	std::vector<Index>::const_iterator pi = e.begin(), pe = e.end();
	for (int pidx=0; pi != pe; ++pi,++pidx)
	{
		if (pidx) out << ",";
		out << *pi;
	}
	return out;
}

Index PosinfoBlock::docno_at( const char* ref) const
{
	if (ref < charptr() || ref > charend())
	{
		throw std::logic_error("illegal posinfo block access -- docno_at");
	}
	char const* rr = ref;
	if (rr == charend()) return 0;
	return docnoFromRelativeIndex( unpackIndex( rr, charend()));
}

unsigned int PosinfoBlock::frequency_at( const char* itr) const
{
	if (itr < charptr() || itr > charend()) throw std::logic_error("illegal posinfo block access -- frequency_at");
	if (itr == charend()) return 0;
	char const* pi = skipIndex( itr, charend());	//... skip docno
	return (*pi == EndPosinfoMarker)?0:unpackIndex( pi, charend());
}

std::vector<Index> PosinfoBlock::positions_at( const char* itr) const
{
	if (itr < charptr() || itr > charend()) throw std::logic_error("illegal posinfo block access -- positions_at");
	std::vector<Index> rt;
	if (itr == charend()) return rt;
	char const* pi = skipIndex( itr, charend());	//... skip docno
	pi = skipIndex( pi, charend());			//... skip ff
	const char* pe = charend();
	while (pi != pe && *pi != EndPosinfoMarker)
	{
		rt.push_back( unpackIndex( pi, pe));
	}
	return rt;
}

const char* PosinfoBlock::end_at( const char* itr) const
{
	if (itr < charptr() || itr > charend()) throw std::logic_error("illegal posinfo block access -- end_at");
	const char* ee = (const char*)std::memchr( itr, EndPosinfoMarker, charend()-itr);
	return ee?ee:charend();
}

bool PosinfoBlock::empty_at( const char* itr) const
{
	if (itr < charptr() || itr > charend()) throw std::logic_error("illegal posinfo block access -- empty_at");
	char const* pi = skipIndex( itr, charend());	//... skip docno
	return (pi == charend() || *pi == EndPosinfoMarker);
}

const char* PosinfoBlock::endOfDoc( const char* ref) const
{
	if (ref == charend()) return 0;
	if (ref < charptr() || ref > charend()) throw std::logic_error("illegal posinfo block access -- endOfDoc");
	char const* rt = (const char*)std::memchr( ref, EndPosinfoMarker, charend()-ref);
	return rt?rt:charend();
}

const char* PosinfoBlock::nextDoc( const char* ref) const
{
	if (ref == charend()) return 0;
	if (ref < charptr() || ref > charend()) throw std::logic_error("illegal posinfo block access -- nextDoc");
	char const* rt = (const char*)std::memchr( ref, EndPosinfoMarker, charend()-ref);
	return (rt)?(rt+1):charend();
}

const char* PosinfoBlock::prevDoc( const char* ref) const
{
	if (ref == charptr()) return 0;
	if (ref < charptr() || ref > charend()) throw std::logic_error("illegal posinfo block access -- prevDoc");
	char const* rt = (const char*)::memrchr( charptr(), EndPosinfoMarker, ref-charptr()-1);
	return (rt >= charptr())?(rt+1):charptr();
}

PosinfoBlockElementMap::const_iterator::const_iterator( const PosinfoBlockElementMap* map_, bool start)
	:m_map_itr(start?map_->map_begin():map_->map_end())
	,m_strings_ref(map_->strings_ptr())
{
	if (m_map_itr != map_->map_end())
	{
		m_elem.init( m_map_itr->first, m_strings_ref);
	}
}

void PosinfoBlockElementMap::define( const Index& idx, const PosinfoBlockElement& pos)
{
	m_map[ idx] = m_strings.size();

	if (pos.size())
	{
		packIndex( m_strings, pos.size());
		PosinfoBlockElement::const_iterator pi = pos.begin(), pe = pos.end();
		for (; pi != pe; ++pi)
		{
			packIndex( m_strings, *pi);
		}
	}
	m_strings.push_back( '\0');
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
	packIndex( blk, relativeIndexFromDocno( docno));	//... relative docno
	packIndex( blk, pos.size());				//... feature frequency (ff)
	std::vector<Index>::const_iterator pi = pos.begin(), pe = pos.end();
	for (; pi != pe; ++pi)
	{
		packIndex( blk, *pi);		// ... [] pos
	}
	DataBlock::append( blk.c_str(), blk.size());
}

void PosinfoBlock::append( const Index& docno, const char* posinfo)
{
	if (size())
	{
		char ch = EndPosinfoMarker;
		DataBlock::append( (const void*)&ch, 1);
	}
	char buf[ 16];
	std::size_t bpos = 0;
	packIndex( buf, bpos, sizeof(buf), relativeIndexFromDocno( docno));

	DataBlock::append( buf, bpos);
	DataBlock::append( (const void*)posinfo, std::strlen(posinfo));
}

void PosinfoBlock::appendPositionsBlock( const char* start, const char* end)
{
	if (size())
	{
		char ch = EndPosinfoMarker;
		DataBlock::append( (const void*)&ch, 1);
	}
	DataBlock::append( (const void*)start, end-start);
}

Index PosinfoBlock::PositionScanner::skip( const Index& pos)
{
	if (pos > std::numeric_limits<int32_t>::max())
	{
		throw std::runtime_error("search position out of range");
	}
	if (m_curpos)
	{
		if (pos == m_curpos)
		{
			return m_curpos;
		}
		else if (pos < m_curpos)
		{
			if (m_lastpos <= pos) return m_curpos;
			m_itr = findIndexAsc( m_start, m_end, (uint32_t)pos);
		}
		else
		{
			m_itr = findIndexAsc( m_itr, m_end, (uint32_t)pos);
		}
	}
	else
	{
		m_itr = findIndexAsc( m_start, m_end, (uint32_t)pos);
	}
	if (m_itr)
	{
		char const* pp = m_itr;
		m_lastpos = pos;
		return (m_curpos = unpackIndex( pp, m_end));
	}
	return 0;
}

PosinfoBlock::PositionScanner PosinfoBlock::positionScanner_at( const char* itr) const
{
	if (itr == charend()) return PositionScanner();
	char const* pi = skipIndex( itr, charend());	//... skip docno
	pi = skipIndex( pi, charend());			//... skip ff
	if (pi == charend()) return PositionScanner( charend(),charend());
	return PositionScanner( pi, endOfDoc( pi));
}

const char* PosinfoBlock::upper_bound( const Index& docno_, const char* lowerbound) const
{
	if (!lowerbound || lowerbound == charend()) return 0;

	if (id() < docno_) throw std::logic_error("called PosinfoBlock::upper_bound with wrong block");

	const char* rt = findStructIndexDesc(
				lowerbound, charend(), EndPosinfoMarker,
				relativeIndexFromDocno( docno_));
	return rt;
}

const char* PosinfoBlock::find( const Index& docno_, const char* lowerbound) const
{
	const char* rt = upper_bound( docno_, lowerbound);
	return (docno_ == docno_at( rt))?rt:0;
}

void PosinfoBlock::setId( const Index& id_)
{
	if (empty())
	{
		DataBlock::setId( id_);
	}
	else
	{
		char const* pp = prevDoc( charend());
		Index maxDocno = docno_at( pp);
		if (maxDocno > id_)
		{
			throw std::runtime_error( "internal: cannot set posinfo block id to a smaller value than the highest docno inserted");
		}
		if (id() != id_)
		{
			// Rewrite document references (first element in variable size record):
			std::string content;
			Index id_diff = id_ - id();
			char const* bi = begin();
			const char* be = end();
			
			while (bi != be)
			{
				packIndex( content, unpackIndex( bi, be) + id_diff);
				const char* blkstart = bi;
				bi = nextDoc( bi);
				content.append( blkstart, bi - blkstart);
			}
			init( id_, content.c_str(), content.size(), content.size());
		}
	}
}

PosinfoBlock PosinfoBlockElementMap::merge( 
		const_iterator ei, const const_iterator& ee, const PosinfoBlock& oldblk)
{
	PosinfoBlock rt;
	rt.setId( oldblk.id());

	char const* old_blkptr = oldblk.begin();
	Index old_docno = oldblk.docno_at( old_blkptr);

	while (ei != ee && old_docno)
	{
		if (ei->docno() <= old_docno)
		{
			if (*ei->ptr())
			{
				//... append only if not empty (empty => delete)
				rt.append( ei->docno(), ei->ptr());
			}
			if (ei->docno() == old_docno)
			{
				//... defined twice -> prefer new entry and ignore old
				old_blkptr = oldblk.nextDoc( old_blkptr);
				old_docno = oldblk.docno_at( old_blkptr);
			}
			++ei;
		}
		else
		{
			if (!oldblk.empty_at( old_blkptr))
			{
				//... append only if not empty (empty => delete)
				rt.appendPositionsBlock( old_blkptr, oldblk.end_at( old_blkptr));
			}
			old_blkptr = oldblk.nextDoc( old_blkptr);
			old_docno = oldblk.docno_at( old_blkptr);
		}
	}
	while (ei != ee)
	{
		if (*ei->ptr())
		{
			//... append only if not empty (empty => delete)
			rt.append( ei->docno(), ei->ptr());
		}
		++ei;
	}
	while (old_docno)
	{
		if (!oldblk.empty_at( old_blkptr))
		{
			//... append only if not empty (empty => delete)
			rt.appendPositionsBlock( old_blkptr, oldblk.end_at( old_blkptr));
		}
		old_blkptr = oldblk.nextDoc( old_blkptr);
		old_docno = oldblk.docno_at( old_blkptr);
	}
	return rt;
}

