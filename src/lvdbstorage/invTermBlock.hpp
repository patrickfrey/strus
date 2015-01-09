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
#ifndef _STRUS_LVDB_INVTERM_BLOCK_HPP_INCLUDED
#define _STRUS_LVDB_INVTERM_BLOCK_HPP_INCLUDED
#include "dataBlock.hpp"
#include "databaseKey.hpp"
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
		Index df;

		Element()
			:typeno(0),termno(0),df(0){}
		Element( const Element& o)
			:typeno(o.typeno),termno(o.termno),df(o.df){}
		Element( const Index& typeno_, const Index& termno_, const Index& df_)
			:typeno(typeno_),termno(termno_),df(df_){}
	};

public:
	explicit InvTermBlock()
		:DataBlock( (char)DatabaseKey::InverseTermIndex){}
	InvTermBlock( const InvTermBlock& o)
		:DataBlock(o){}
	InvTermBlock( const Index& id_, const void* ptr_, std::size_t size_)
		:DataBlock( (char)DatabaseKey::InverseTermIndex, id_, ptr_, size_){}

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

	void append( const Index& typeno, const Index& termno, const Index& df);
};

}//namespace
#endif

