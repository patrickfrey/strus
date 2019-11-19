/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_ITERATOR_JOIN_HPP_INCLUDED
#define _STRUS_ITERATOR_JOIN_HPP_INCLUDED
#include "strus/postingIteratorInterface.hpp"
#include <stdexcept>
#include <limits>

namespace strus
{

/// \brief Iterator interface for join iterators with the common part implemented
class IteratorJoin
	:public PostingIteratorInterface
{
public:
	virtual ~IteratorJoin(){}

	virtual int frequency()
	{
		Index idx=0;
		unsigned int rt = 0;
		for (;0!=(idx=skipPos( idx))
			&& rt < (unsigned int)std::numeric_limits<short>::max();
				++idx,++rt){}
		return rt;
	}
};

}//namespace
#endif


