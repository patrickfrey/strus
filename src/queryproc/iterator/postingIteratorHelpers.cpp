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
/// \brief Implementation of helper functions shared by iterators
#include "iterator/postingIteratorHelpers.hpp"
#include <sstream>
#include <iostream>

using namespace strus;

Index strus::getFirstAllMatchDocno( const std::vector<PostingIteratorReference>& ar, Index docno_iter)
{
	for (;;)
	{
		std::vector<PostingIteratorReference>::const_iterator pi = ar.begin(), pe = ar.end();
		if (pi == pe)
		{
			return 0;
		}
		Index docno_first = (*pi)->skipDoc( docno_iter);
		if (!docno_first)
		{
			return 0;
		}
		bool match = true;
		for (++pi; pi != pe; ++pi)
		{
			Index docno_next = (*pi)->skipDoc( docno_first);
			if (!docno_next)
			{
				return 0;
			}
			if (docno_next != docno_first)
			{
				match = false;
				docno_iter = docno_next;
				break;
			}
		}
		if (match)
		{
			return docno_first;
		}
	}
}

void strus::encodeInteger( std::string& buf, int val)
{
	std::ostringstream num;
	num << val;
	buf.append( num.str());
}

