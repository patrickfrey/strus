/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "invTermBlock.hpp"
#include "indexPacker.hpp"

using namespace strus;

InvTermBlock::Element InvTermBlock::element_at( const char* itr) const
{
	InvTermBlock::Element rt;
	if (itr == charend()) return rt;
	char const* ri = itr;
	rt.typeno = unpackIndex( ri, charend());
	rt.termno = unpackIndex( ri, charend());
	rt.ff = unpackIndex( ri, charend());
	rt.firstpos = unpackIndex( ri, charend());
	return rt;
}

const char* InvTermBlock::next( const char* ref) const
{
	if (ref == charend()) return 0;
	char const* ri = ref;
	ri = skipIndex( ri, charend());
	ri = skipIndex( ri, charend());
	ri = skipIndex( ri, charend());
	ri = skipIndex( ri, charend());
	return ri;
}

void InvTermBlock::append( const Index& typeno, const Index& termno, const Index& ff, const Index& firstpos)
{
	std::string elem;
	packIndex( elem, typeno);
	packIndex( elem, termno);
	packIndex( elem, ff);
	packIndex( elem, firstpos);
	DataBlock::append( elem.c_str(), elem.size());
}

