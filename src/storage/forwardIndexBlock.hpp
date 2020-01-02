/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_FORWARD_INDEX_BLOCK_HPP_INCLUDED
#define _STRUS_STORAGE_FORWARD_INDEX_BLOCK_HPP_INCLUDED
#include "dataBlock.hpp"
#include "private/localStructAllocator.hpp"
#include <string>
#include <map>

namespace strus {

/// \class ForwardIndexBlock
/// \brief Block of term occurrence positions
class ForwardIndexBlock
	:public DataBlock
{
public:
	enum {
		MaxBlockTokens=128
	};

public:
	ForwardIndexBlock(){}
	ForwardIndexBlock( const ForwardIndexBlock& o)
		:DataBlock(o){}
	ForwardIndexBlock( strus::Index id_, const void* ptr_, std::size_t size_)
		:DataBlock( id_, ptr_, size_)
	{
		initFrame();
	}

	ForwardIndexBlock& operator=( const ForwardIndexBlock& o)
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

	void setId( strus::Index id_);
	Index position_at( const char* ref) const;
	std::string value_at( const char* ref) const;

	Index relativeIndexFromPosition( strus::Index pos_) const {return id()-pos_+1;}
	Index positionFromRelativeIndex( strus::Index rel_) const {return id()-rel_+1;}

	const char* nextItem( const char* ref) const;
	const char* prevItem( const char* ref) const;

	const char* find( strus::Index pos_, const char* lowerbound) const;
	const char* upper_bound( strus::Index pos_, const char* lowerbound) const;

	void append( strus::Index pos, const std::string& item);

	class const_iterator
	{
	public:
		const_iterator( const ForwardIndexBlock* blk_, const char* ref_)
			:m_blk(blk_),m_ref(ref_){}
		const_iterator( const const_iterator& o)
			:m_blk(o.m_blk),m_ref(o.m_ref){}

		std::string operator*() const			{return m_blk->value_at( m_ref);}

		const_iterator& operator++()			{m_ref = m_blk->nextItem( m_ref); return *this;}
		const_iterator operator++(int)			{const_iterator rt(m_blk,m_ref); m_ref = m_blk->nextItem( m_ref); return rt;}

		bool operator==( const const_iterator& o) const	{return m_ref == o.m_ref;}
		bool operator!=( const const_iterator& o) const	{return m_ref != o.m_ref;}

	private:
		const ForwardIndexBlock* m_blk;
		char const* m_ref;
	};

	const_iterator begin() const
	{
		return const_iterator( this, charptr());
	}
	const_iterator end() const
	{
		return const_iterator( this, charend());
	}
	void initFrame(){}
};

}
#endif

