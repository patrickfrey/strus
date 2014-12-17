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
#ifndef _STRUS_LVDB_DOCNO_BLOCK_HPP_INCLUDED
#define _STRUS_LVDB_DOCNO_BLOCK_HPP_INCLUDED
#include "strus/index.hpp"
#include "floatConversions.hpp"
#include "fixedSizeRecordBlock.hpp"
#include "dataBlock.hpp"
#include "databaseKey.hpp"
#include <iostream>
#include <map>
#include <stdint.h>

namespace strus {

class DocnoBlockElement
{
public:
	DocnoBlockElement() :m_docno(0),m_ff(0),m_weight(0.0){}
	DocnoBlockElement( Index docno_, unsigned int ff_, float weight_);
	DocnoBlockElement( const DocnoBlockElement& o);

	float weight() const;

	unsigned int ff() const
	{
		return m_ff;
	}

	Index docno() const
	{
		return m_docno;
	}

	bool empty() const
	{
		return !m_ff;
	}

private:
	enum {Max_ff=0xffFF};
	Index m_docno;
	uint16_t m_ff;
	strus::float16_t m_weight;	///< IEEE 754 half-precision binary floating-point format: binary16
};

std::ostream& operator<< (std::ostream& out, const DocnoBlockElement& e);



class DocnoBlock
	:public FixedSizeRecordBlock<DocnoBlockElement>
{
public:
	enum {
		NofBlockElements=128
	};

public:
	DocnoBlock()
		:FixedSizeRecordBlock<DocnoBlockElement>( (char)DatabaseKey::DocnoBlockPrefix){}
	DocnoBlock( const DocnoBlockElement* ar_, std::size_t arsize_)
		:FixedSizeRecordBlock<DocnoBlockElement>( (char)DatabaseKey::DocnoBlockPrefix, ar_, arsize_){}
	DocnoBlock( const DocnoBlock& o)
		:FixedSizeRecordBlock<DocnoBlockElement>( o){}

	DocnoBlock& operator=( const DocnoBlock& o)
	{
		FixedSizeRecordBlock<DocnoBlockElement>::operator =(o);
		return *this;
	}

	const DocnoBlockElement* find( const Index& docno_, const DocnoBlockElement* lowerbound) const;
	const DocnoBlockElement* upper_bound( const Index& docno_, const DocnoBlockElement* lowerbound) const;

	bool full() const
	{
		return nofElements() >= NofBlockElements;
	}
	void append( const Index& docno_, const DocnoBlockElement& elem)
	{
		if (docno_ == 0) throw std::logic_error( "inserting docno NULL in docno block");
		if (elem.docno() != docno_) throw std::logic_error( "docno block element docid does not match");
		push_back( elem);
	}
	/// \brief Check if the address 'docno_', if it exists, is in this block.
	bool isThisBlockAddress( const Index& docno_) const
	{
		return (docno_ <= id() && docno_ > first().docno());
	}
	/// \brief Check if the address 'docno_', if it exists, is in the following block we can get with 'leveldb::Iterator::Next()' or not
	bool isFollowBlockAddress( const Index& docno_) const
	{
		return (docno_ > id() && docno_ < id() + NofBlockElements);
	}
};



class DocnoBlockElementMap
	:public std::map<Index,DocnoBlockElement>
{
public:
	DocnoBlockElementMap(){}
	DocnoBlockElementMap( const DocnoBlockElementMap& o)
		:std::map<Index,DocnoBlockElement>(o){}

	void define( const Index& idx, const DocnoBlockElement& elem)
	{
		std::map<Index,DocnoBlockElement>::operator[]( idx) = elem;
	}
	Index lastInsertBlockId() const
	{
		return rbegin()->first;
	}

	class IteratorElement
	{
	public:
		explicit IteratorElement( const std::map<Index,DocnoBlockElement>::const_iterator& itr_)
			:m_itr(itr_){}
		IteratorElement& operator=( const std::map<Index,DocnoBlockElement>::const_iterator& itr_)
		{
			m_itr = itr_;
			return *this;
		}

		const Index& docno() const		{return m_itr->first;}
		const DocnoBlockElement& value() const	{return m_itr->second;}

	private:
		std::map<Index,DocnoBlockElement>::const_iterator m_itr;
	};

	class const_iterator
	{
	public:
		const_iterator( const const_iterator& o)
			:m_itr(o.m_itr),m_elem(o.m_itr){}

		explicit const_iterator( const std::map<Index,DocnoBlockElement>::const_iterator& itr_)
			:m_itr(itr_),m_elem(itr_){}

		bool operator==( const const_iterator& o) const	{return m_itr==o.m_itr;}
		bool operator!=( const const_iterator& o) const	{return m_itr!=o.m_itr;}

		const_iterator& operator++()			{++m_itr; m_elem = m_itr; return *this;}
		const_iterator operator++(int)			{const_iterator rt(*this); ++m_itr; m_elem = m_itr; return rt;}

		const IteratorElement& operator*() const	{return m_elem;}
		const IteratorElement* operator->() const	{return &m_elem;}

	private:
		std::map<Index,DocnoBlockElement>::const_iterator m_itr;
		IteratorElement m_elem;
	};

	const_iterator begin() const
	{
		return const_iterator( std::map<Index,DocnoBlockElement>::begin());
	}
	const_iterator end() const
	{
		return const_iterator( std::map<Index,DocnoBlockElement>::end());
	}

	static DocnoBlock merge( const_iterator ei, const const_iterator& ee, const DocnoBlock& oldblk);
};

}
#endif

