/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "booleanBlock.hpp"
#include "indexPacker.hpp"
#include "private/internationalization.hpp"
#include <limits>

using namespace strus;

void BooleanBlock::Node::init( const Index& from_, const Index& to_)
{
	if (from_ == to_)
	{
		type = PairNode;
		alt.elemno2 = 0;
		elemno = from_;
	}
	else
	{
		type = DiffNode;
		alt.diff = to_ - from_;
		elemno = to_;
	}
}

void BooleanBlock::Node::normalize()
{
	switch (type)
	{
		case DiffNode:
			if (alt.diff == 0)
			{
				type = PairNode;
				alt.elemno2 = 0;
			}
			break;
		case PairNode:
			if (alt.elemno2 + 1 == elemno)
			{
				type = DiffNode;
				alt.diff = 1;
			}
	}
}

bool BooleanBlock::Node::matches( const Index& elemno_) const
{
	switch (type)
	{
		case DiffNode:
			return (elemno_ <= elemno && elemno_ >= elemno - alt.diff);
		case PairNode:
			return (elemno_ == elemno || elemno_ == alt.elemno2);
	}
	return false;
}

bool BooleanBlock::Node::tryExpandRange( const Index& to_)
{
	switch (type)
	{
		case DiffNode:
			if (elemno >= to_) throw std::runtime_error( _TXT( "internal: call try expand range with node end within node"));
			alt.diff += (to_ - elemno);
			elemno = to_;
			return true;

		case PairNode:
			if (alt.elemno2)
			{
				return false;
			}
			else
			{
				init( elemno, to_);
				return true;
			}
	}
	return false;
}

bool BooleanBlock::Node::tryAddElem( const Index& elemno_)
{
	switch (type)
	{
		case DiffNode:
			if (elemno_ <= elemno + 1 && elemno_ >= elemno - alt.diff - 1)
			{
				if (elemno_ == elemno + 1)
				{
					++elemno;
				}
				else if (elemno_ == elemno - alt.diff - 1)
				{
					++alt.diff;
				}
				normalize();
				return true;
			}
			break;

		case PairNode:
			if (alt.elemno2)
			{
				if (elemno == elemno_ || alt.elemno2 == elemno_) return true;
			}
			else
			{
				if (elemno_ <= elemno)
				{
					if (elemno_ == elemno) return true;
					alt.elemno2 = elemno_;
				}
				else
				{
					alt.elemno2 = elemno;
					elemno = elemno_;
				}
				normalize();
				return true;
			}
	}
	return false;
}

Index BooleanBlock::Node::getFirstElem() const
{
	switch (type)
	{
		case DiffNode:
			return elemno - alt.diff;

		case PairNode:
			return alt.elemno2?alt.elemno2:elemno;
	}
	return 0;
}

Index BooleanBlock::Node::getLastElem() const
{
	return elemno;
}

Index BooleanBlock::Node::getNextElem( const Index& elemno_) const
{
	switch (type)
	{
		case DiffNode:
			return (elemno_ < elemno)?(elemno_+1):0;

		case PairNode:
			if (elemno_ == alt.elemno2 && alt.elemno2)
			{
				return elemno;
			}
			else
			{
				return 0;
			}
	}
	return 0;
}

Index BooleanBlock::Node::getUpperBound( const Index& elemno_) const
{
	switch (type)
	{
		case DiffNode:
			return (elemno_ <= elemno && elemno_ >= elemno - alt.diff)?elemno_:(elemno - alt.diff);

		case PairNode:
			if (elemno_ <= elemno)
			{
				if (elemno_ <= alt.elemno2 && alt.elemno2)
				{
					return alt.elemno2;
				}
				else
				{
					return elemno;
				}
			}
			else
			{
				return 0;
			}
	}
	return 0;
}

void BooleanBlock::Node::getLastRange( Index& from_, Index& to_) const
{
	switch (type)
	{
		case DiffNode:
			from_ = elemno - alt.diff;
			to_ = elemno;
			break;

		case PairNode:
			if (alt.elemno2 && elemno == alt.elemno2+1)
			{
				from_ = alt.elemno2;
				to_ = elemno;
			}
			else
			{
				from_ = to_ = elemno;
			}
			break;
	}
}

void BooleanBlock::initFrame()
{
	if (!empty())
	{
		const Node* nd = (const Node*)ptr();
		m_first = nd->getFirstElem();
	}
}

Index BooleanBlock::getFirst( NodeCursor& cursor) const
{
	cursor.reset();
	if (empty()) return 0;
	const Node* nd = (const Node*)ptr();
	return cursor.elemno = nd->getFirstElem();
}

Index BooleanBlock::getLast() const
{
	if (empty()) return 0;
	std::size_t nodearsize = (size() / sizeof(Node));
	const Node* nd = (const Node*)ptr() + nodearsize-1;
	return nd->getLastElem();
}

Index BooleanBlock::getLast( NodeCursor& cursor) const
{
	if (empty()) return 0;
	std::size_t nodearsize = (size() / sizeof(Node));
	cursor.idx = nodearsize-1;
	const Node* nd = (const Node*)ptr() + cursor.idx;
	return cursor.elemno = nd->getLastElem();
}

Index BooleanBlock::getNext( NodeCursor& cursor) const
{
	std::size_t nodearsize = (size() / sizeof(Node));
	if (cursor.idx >= nodearsize) return cursor.elemno=0;
	Node const* nd = (const Node*)ptr() + cursor.idx;
	cursor.elemno = nd->getNextElem( cursor.elemno);
	if (!cursor.elemno)
	{
		++nd;
		++cursor.idx;
		if (cursor.idx >= nodearsize) return cursor.elemno=0;

		cursor.elemno = nd->getFirstElem();
	}
	return cursor.elemno;
}

bool BooleanBlock::getFirstRange( NodeCursor& cursor, Index& from_, Index& to_) const
{
	cursor.reset();
	return getNextRange( cursor, from_, to_);
}

bool BooleanBlock::getNextRange( NodeCursor& cursor, Index& from_, Index& to_) const
{
	std::size_t nodearsize = (size() / sizeof(Node));
	if (cursor.idx >= nodearsize) return false;
	Node const* nd = (const Node*)ptr() + cursor.idx;
	switch (nd->type)
	{
		case BooleanBlock::Node::DiffNode:
			cursor.elemno = 0;
			++cursor.idx;
			nd->getLastRange( from_, to_);
			return true;

		case BooleanBlock::Node::PairNode:
			if (cursor.elemno == 0 && nd->alt.elemno2)
			{
				cursor.elemno = nd->alt.elemno2;
				from_ = to_ = nd->alt.elemno2;
				return true;
			}
			else
			{
				cursor.elemno = 0;
				++cursor.idx;
				nd->getLastRange( from_, to_);
				return true;
			}
	}
	return false;
}

Index BooleanBlock::skip( const Index& elemno_, NodeCursor& cursor) const
{
	std::size_t nodearsize = (size() / sizeof(Node));
	Node const* nd = ((Node const*)ptr()) + cursor.idx;
	if (cursor.idx >= nodearsize)
	{
		cursor.idx = 0;
		if (((const Node*)ptr())->elemno >= elemno_)
		{
			return ((const Node*)ptr())->getUpperBound( elemno_);
		}
	}
	else if (nd->elemno >= elemno_)
	{
		if (nd->matches( elemno_))
		{
			return cursor.elemno = elemno_;
		}
		while (cursor.idx > 0 && nd->elemno > elemno_)
		{
			cursor.idx >>= 1;
			nd = ((Node const*)ptr()) + cursor.idx;
		}
		if (nd->elemno >= elemno_)
		{
			return nd->getUpperBound( elemno_);
		}
	}
	std::size_t fibres = 0, fib1 = 1, fib2 = 1, ii = cursor.idx+1, nn = nodearsize;
	while (ii < nn && ((const Node*)ptr())[ ii].elemno < elemno_)
	{
		fibres = fib1 + fib2;
		ii += fibres;
		fib1 = fib2;
		fib2 = fibres;
	}
	nd = ((Node const*)ptr()) + ii - fibres;
	for (ii -= fibres; ii < nn && nd->elemno < elemno_; ++ii,++nd){}
	if (ii < nn)
	{
		Index rt = nd->getUpperBound( elemno_);
		if (rt)
		{
			cursor.idx = ii;
			return cursor.elemno = rt;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}

void BooleanBlock::defineElement( const Index& elemno)
{
	defineRange( elemno, 0);
}


bool BooleanBlock::joinRange( Index& from_, Index& to_, const Index& from2_, const Index& to2_)
{
	if (from_ <= from2_)
	{
		if (to_ >= from2_)
		{
			if (to_ <= to2_)
			{
				//... case [from_][from2_][to_][to2_]
				to_ = to2_;
				return true;
			}
			else
			{
				//... case [from_][from2_][to2_][to_]
				// => first range covers 2nd => nothing to do
				return true;
			}
		}
		else
		{
			//...[from_][to_][from2_][to2_]
			if (to_+1 == from2_)
			{
				to_ = to2_;
				return true;
			}
			else
			{
				//...[from_][to_] ... [from2_][to2_] not joinable to one
				return false;
			}
		}
	}
	else
	{
		//...[from2_][from_] => swap szenario
		if (to2_ >= from_)
		{
			if (to2_ <= to_)
			{
				//... case [from2_][from_][to2_][to_]
				from_ = from2_;
				return true;
			}
			else
			{
				//... case [from2_][from_][to_][to2_]
				from_ = from2_;
				to_ = to2_;
				return true;
			}
		}
		else
		{
			//...[from2_][to2_][from_][to_]
			if (to2_+1 == from_)
			{
				from_ = from2_;
				return true;
			}
			else
			{
				//...[from2_][to2_] ... [from_][to_] not joinable to one
				return false;
			}
		}
	}
}

void BooleanBlock::defineRange( const Index& elemno, const Index& rangesize)
{
	Index from_ = 0;
	Index to_ = 0;

	std::size_t nodearsize = (size() / sizeof(Node));
	if (nodearsize)
	{
		Node* nd = (Node*)ptr() + nodearsize - 1;
		nd->getLastRange( from_, to_);

		if (elemno < from_)
		{
			throw std::runtime_error( _TXT( "internal: ranges not appended in order in boolean block"));
		}
		if (elemno <= to_)
		{
			// [from_ <= elemno <= to_]
			//... overlapping ranges => join them
			if (to_ < elemno + rangesize)
			{
				// [from_ <= elemno <= to_ < elemno+rangesize]
				if (!nd->tryExpandRange( elemno+rangesize))
				{
					Node newnod;
					newnod.init( elemno, elemno + rangesize);
					append( &newnod, sizeof(newnod));
				}
			}
			else
			{
				// [from_ <= elemno <= elemno+rangesize <= to_]
				//... new range inside old one
			}
		}
		else
		{
			//... not overlapping with last range => add new
			if (rangesize == 0 && nd->tryAddElem( elemno))
			{}
			else
			{
				Node newnod;
				newnod.init( elemno, elemno + rangesize);
				append( &newnod, sizeof(newnod));
			}
		}
	}
	else
	{
		//... first range => add new
		Node newnod;
		newnod.init( elemno, elemno + rangesize);
		append( &newnod, sizeof(newnod));
		initFrame();
	}
}

void BooleanBlock::merge( 
		const BooleanBlock& blk1,
		const BooleanBlock& blk2,
		BooleanBlock& newblk)
{
	newblk.clear();
	newblk.setId( blk1.id() > blk2.id() ? blk1.id() : blk2.id());

	NodeCursor cursor1;
	strus::Index from1;
	strus::Index to1;

	NodeCursor cursor2;
	strus::Index from2;
	strus::Index to2;

	bool hasMore1 = blk1.getFirstRange( cursor1, from1, to1);
	bool hasMore2 = blk2.getFirstRange( cursor2, from2, to2);
	while (hasMore1 && hasMore2)
	{
		strus::Index from;
		strus::Index to;

		if (from1 <= from2)
		{
			if (to1 >= from2)
			{
				// ... case overlap ( [ ) ]  or ( [ ] )
				from = from1;
				to = to1 > to2 ? to1 : to2;
				hasMore1 = blk1.getNextRange( cursor1, from1, to1);
				hasMore2 = blk2.getNextRange( cursor2, from2, to2);
			}
			else
			{
				// case choose first ( ) .. [ ]
				from = from1;
				to = to1;
				hasMore1 = blk1.getNextRange( cursor1, from1, to1);
			}
		}
		else /*from1 > from2*/
		{
			if (from1 > to2)
			{
				// case choose second [ ] .. ( )
				from = from2;
				to = to2;
				hasMore2 = blk2.getNextRange( cursor2, from2, to2);
			}
			else/*from1 <= to2*/
			{
				// ... case overlap [ ( ) ]  of [ ( ] )
				from = from2;
				to = to1 > to2 ? to1 : to2;
				hasMore1 = blk1.getNextRange( cursor1, from1, to1);
				hasMore2 = blk2.getNextRange( cursor2, from2, to2);
			}
		}
		newblk.defineRange( from, to - from);
	}
	while (hasMore1)
	{
		newblk.defineRange( from1, to1 - from1);
		hasMore1 = blk1.getNextRange( cursor1, from1, to1);
	}
	while (hasMore2)
	{
		newblk.defineRange( from2, to2 - from2);
		hasMore2 = blk2.getNextRange( cursor2, from2, to2);
	}
}


void BooleanBlock::merge( 
		std::vector<MergeRange>::const_iterator ei,
		const std::vector<MergeRange>::const_iterator& ee,
		const BooleanBlock& oldblk,
		BooleanBlock& newblk)
{
	newblk.clear();
	newblk.setId( oldblk.id());
	NodeCursor cursor;

	Index old_from = 0;
	Index old_to = 0;
	bool old_haselem = oldblk.getNextRange( cursor, old_from, old_to);

	while (ei != ee && old_haselem)
	{
		if (ei->isMember)
		{
			//... define element
			if (BooleanBlock::joinRange( old_from, old_to, ei->from, ei->to))
			{
				++ei;
			}
			else
			{
				if (ei->from < old_from)
				{
					newblk.defineRange( ei->from, ei->to - ei->from);
					++ei;
				}
				else
				{
					newblk.defineRange( old_from, old_to - old_from);
					old_haselem = oldblk.getNextRange( cursor, old_from, old_to);
				}
			}
		}
		else
		{
			//... delete element
			if (old_from <= ei->from)
			{
				// => case [old.from][new.from]
				if (old_to >= ei->from)
				{
					// .... that is inside the current range
					// => case [old.from][new.from][old.to]
					if (old_from < ei->from)
					{
						// => case [old.from][new.from][old.to]
						newblk.defineRange( old_from, ei->from - old_from - 1);
					}
					if (old_to > ei->to)
					{
						// => case [old.from][new.from][new.to][old.to]
						old_from = ei->to + 1;
						++ei;
					}
					else
					{
						old_haselem = oldblk.getNextRange( cursor, old_from, old_to);
						++ei;
					}
				}
				else
				{
					// .... that does not touch the old block
					// => case [old.from][old.to][new.from]
					newblk.defineRange( old_from, old_to - old_from);
					old_haselem = oldblk.getNextRange( cursor, old_from, old_to);
				}
			}
			else if (ei->to >= old_from)
			{
				// => case [new.from][old.from][new.to]
				if (ei->to >= old_to)
				{
					// => case [new.from][old.from][old.to][new.to]
					//... deleted elements are covering old range completely
					old_haselem = oldblk.getNextRange( cursor, old_from, old_to);
				}
				else
				{
					// => case [new.from][old.from][new.to][old.to]
					old_from = ei->to + 1;
				}
			}
			else
			{
				//... delete undefined element => ignore
				++ei;
			}
		}
	}
	while (ei != ee)
	{
		if (ei->isMember)
		{
			newblk.defineRange( ei->from, ei->to - ei->from);
		}
		++ei;
	}
	while (old_haselem)
	{
		newblk.defineRange( old_from, old_to - old_from);
		old_haselem = oldblk.getNextRange( cursor, old_from, old_to);
	}
}

void BooleanBlock::check() const
{
	NodeCursor cursor;

	Index rangemin;
	Index rangemax;
	Index prevmax = 0;

	while (getNextRange( cursor, rangemin, rangemax))
	{
		if (rangemin <= 0 || rangemax <= 0 || rangemin > rangemax || rangemax > id() || rangemin <= prevmax)
		{
			throw std::runtime_error( _TXT( "created illegal boolean block"));
		}
	}
}

