/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013 Patrick Frey

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
#ifndef _STRUS_STORAGE_ITERATOR_HPP_INCLUDED
#define _STRUS_STORAGE_ITERATOR_HPP_INCLUDED
#include "position.hpp"
#include "storage.hpp"
#include <string>
#include <map>

namespace strus
{

struct PositionIterator
	:public PositionChunk
{
	virtual ~PositionIterator(){}
	virtual bool fetch()=0;

	Position get() const
	{
		if (m_posidx >= m_posarsize) return 0;
		return m_posar[ m_posidx];
	}

	void next()
	{
		if (m_posidx >= m_posarsize)
		{
			fetch();
			m_eof = (m_posidx >= m_posarsize);
		}
		else
		{
			m_posidx++;
		}
	}
};

class StoragePositionIterator
	:public PositionIterator
{
public:
	StoragePositionIterator( Storage* storage_, const TermNumber& termnum_);

	virtual ~StoragePositionIterator();
	virtual bool fetch();

private:
	Storage* m_storage;
	TermNumber m_termnum;
};


class UnionPositionIterator
	:public PositionIterator
{
public:
	UnionPositionIterator( PositionIterator* term1_, PositionIterator* term2_);

	virtual ~UnionPositionIterator();
	virtual bool fetch();

private:
	void getNextChunk();

private:
	PositionIterator* m_term1;
	PositionIterator* m_term2;
	std::size_t m_memblocksize;
};


class IntersectionCutPositionIterator
	:public PositionIterator
{
public:
	IntersectionCutPositionIterator( PositionIterator* ths_, PositionIterator* oth_, int rangestart_, int range_, PositionIterator* cut_);

	virtual ~IntersectionCutPositionIterator();
	virtual bool fetch();

private:
	void getNextChunk();

private:
	PositionIterator* m_ths;
	PositionIterator* m_oth;
	PositionIterator* m_cut;
	int m_rangestart;
	int m_range;
	std::size_t m_memblocksize;
};

}//namespace

#endif


