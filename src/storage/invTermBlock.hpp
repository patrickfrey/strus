/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#ifndef _STRUS_STORAGE_INVTERM_BLOCK_HPP_INCLUDED
#define _STRUS_STORAGE_INVTERM_BLOCK_HPP_INCLUDED
#include "dataBlock.hpp"
#include <vector>
#include <map>

namespace strus {

/// \class InvTermBlock
/// \brief Block of inverse term occurrencies
class InvTermBlock
	:public DataBlock
{
public:
	struct Element
	{
		Index typeno;
		Index termno;
		Index ff;
		Index firstpos;

		Element()
			:typeno(0),termno(0),ff(0),firstpos(0){}
		Element( const Element& o)
			:typeno(o.typeno),termno(o.termno),ff(o.ff),firstpos(o.firstpos){}
		Element( const Index& typeno_, const Index& termno_, const Index& ff_, const Index& firstpos_)
			:typeno(typeno_),termno(termno_),ff(ff_),firstpos(firstpos_){}
	};

public:
	InvTermBlock(){}
	InvTermBlock( const InvTermBlock& o)
		:DataBlock(o){}
	InvTermBlock( const Index& id_, const void* ptr_, std::size_t size_, bool allocated=false)
		:DataBlock( id_, ptr_, size_, allocated){}

	InvTermBlock& operator=( const InvTermBlock& o)
	{
		DataBlock::operator =(o);
		return *this;
	}

	Element element_at( const char* itr) const;

	const char* begin() const
	{
		return charptr();
	}
	const char* end() const
	{
		return charend();
	}

	const char* next( const char* ref) const;

	void append( const Index& typeno, const Index& termno, const Index& ff, const Index& firstpos);
};

}//namespace
#endif

