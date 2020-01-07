/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STRUCTURE_BLOCK_DECLARATION_HPP_INCLUDED
#define _STRUS_STRUCTURE_BLOCK_DECLARATION_HPP_INCLUDED
#include "strus/index.hpp"

namespace strus {

struct StructBlockDeclaration
{
	strus::Index structno;
	strus::IndexRange src;
	strus::IndexRange sink;

	StructBlockDeclaration( strus::Index structno_, const strus::IndexRange& src_, const strus::IndexRange& sink_)
		:structno(structno_),src(src_),sink(sink_){}
	StructBlockDeclaration( const StructBlockDeclaration& o)
		:structno(o.structno),src(o.src),sink(o.sink){}

	bool operator < ( const StructBlockDeclaration& o) const
	{
		return (structno == o.structno)
			? (src == o.src
				? (sink < o.sink)
				: src < o.src)
			: (structno < o.structno);
	}
	bool operator == ( const StructBlockDeclaration& o) const
	{
		return (structno == o.structno && src == o.src && sink == o.sink);
	}
	bool operator != ( const StructBlockDeclaration& o) const
	{
		return !operator==(o);
	}
};

}//namespace
#endif

