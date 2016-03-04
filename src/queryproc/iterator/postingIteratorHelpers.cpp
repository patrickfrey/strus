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
/// \brief Implementation of helper functions shared by iterators
#include "postingIteratorHelpers.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <sstream>
#include <cstring>

using namespace strus;

typedef Reference< PostingIteratorInterface> PostingIteratorReference;

Index strus::getFirstAllMatchDocno(
		std::vector<PostingIteratorReference>& args,
		Index docno,
		bool allowEmpty)
{
	if (args.empty()) return 0;

	Index docno_iter = docno;
	std::vector<PostingIteratorReference>::iterator ae = args.end();
	for (;;)
	{
		std::vector<PostingIteratorReference>::iterator ai = args.begin();

		docno_iter = (*ai)->skipDocCandidate( docno_iter);
		if (docno_iter == 0)
		{
			return 0;
		}
		Index max_docno = docno_iter;
		bool failed = false;
		for (++ai; ai != ae; ++ai)
		{
			Index docno_next = (*ai)->skipDocCandidate( docno_iter);
			if (docno_next == 0)
			{
				return 0;
			}
			if (docno_next != docno_iter)
			{
				if (docno_next > max_docno)
				{
					max_docno = docno_next;
					docno_iter = docno_next;
					failed = true;
				}
			}
		}
		if (!failed)
		{
			if (!allowEmpty)
			{
				ai = args.begin();
				for (; ai != ae && (*ai)->skipPos(0); ++ai){}

				if (ai != ae)
				{
					++docno_iter;
					continue;
				}
			}
			return docno_iter;
		}
	}
}

void strus::encodeInteger( std::string& buf, int val)
{
	std::ostringstream num;
	num << val;
	buf.append( num.str());
}

