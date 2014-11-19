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
#ifndef _STRUS_LVDB_FIXED_SIZE_RECORD_BLOCK_HPP_INCLUDED
#define _STRUS_LVDB_FIXED_SIZE_RECORD_BLOCK_HPP_INCLUDED
#include "dataBlock.hpp"

namespace strus {

template <class Element>
class FixedSizeRecordBlock
	:public DataBlock
{
public:
	explicit FixedSizeRecordBlock( char blocktype_)
		:DataBlock( blocktype_){}
	FixedSizeRecordBlock( char blocktype_, const Element* ar_, std::size_t arsize_)
		:DataBlock( blocktype_, arsize_?ar_[arsize_-1].docno():0, (const void*)ar_, arsize_*sizeof(*ar_)){}
	FixedSizeRecordBlock( const FixedSizeRecordBlock& o)
		:DataBlock( o){}

	std::size_t nofElements() const
	{
		return size()/sizeof(Element);
	}

	const Element* ptr() const
	{
		return (const Element*)DataBlock::ptr();
	}

	void init( const Element* ar_, std::size_t arsize_)
	{
		DataBlock::init( arsize_?ar_[arsize_-1].docno():0, ar_, arsize_*sizeof(*ar_));
	}

	void push_back( const Element& e)
	{
		DataBlock::append( &e, sizeof(e));
	}

	const Element& back() const
	{
		return ptr()[ nofElements()-1];
	}

	const Element& first() const
	{
		return ptr()[ 0];
	}

	class const_iterator
	{
	public:
		explicit const_iterator( const Element* ar_=0)
			:m_itr(ar_){}
		const_iterator( const const_iterator& o)
			:m_itr(o.m_itr){}

		const_iterator& operator++()				{++m_itr; return *this;}
		const_iterator operator++(int)				{const_iterator rt=m_itr; ++m_itr; return rt;}
		const Element& operator*() const			{return *m_itr;}
		const Element* operator->() const			{return m_itr;}
		std::size_t operator-( const const_iterator& o)	const	{return m_itr-o.m_itr;}

		bool operator==( const const_iterator& o)		{return m_itr==o.m_itr;}
		bool operator!=( const const_iterator& o)		{return m_itr!=o.m_itr;}

		bool initialized() const				{return !!m_itr;}
		void clear()						{m_itr = 0;}

	private:
		Element const* m_itr;
	};

	const_iterator begin() const
	{
		return const_iterator( ptr());
	}

	const_iterator end() const
	{
		return const_iterator( ptr()+nofElements());
	}
};

}
#endif

