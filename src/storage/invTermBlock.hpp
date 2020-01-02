/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
		strus::Index typeno;
		strus::Index termno;
		strus::Index ff;
		strus::Index firstpos;

		Element()
			:typeno(0),termno(0),ff(0),firstpos(0){}
		Element( const Element& o)
			:typeno(o.typeno),termno(o.termno),ff(o.ff),firstpos(o.firstpos){}
		Element( strus::Index typeno_, strus::Index termno_, strus::Index ff_, strus::Index firstpos_)
			:typeno(typeno_),termno(termno_),ff(ff_),firstpos(firstpos_){}
	};

public:
	InvTermBlock(){}
	InvTermBlock( const InvTermBlock& o)
		:DataBlock(o)
	{
		initFrame();
	}
	InvTermBlock( strus::Index id_, const void* ptr_, std::size_t size_, bool allocated=false)
		:DataBlock( id_, ptr_, size_, allocated)
	{
		initFrame();
	}

	InvTermBlock& operator=( const InvTermBlock& o)
	{
		DataBlock::operator =(o);
		initFrame();
		return *this;
	}
	void swap( DataBlock& o)
	{
		DataBlock::swap( o);
		initFrame();
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

	void append( strus::Index typeno, strus::Index termno, strus::Index ff, strus::Index firstpos);

	void initFrame(){}
};

}//namespace
#endif

