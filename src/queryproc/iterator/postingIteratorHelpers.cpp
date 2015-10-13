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
#include "postingIteratorHelpers.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <sstream>
#include <iostream>

using namespace strus;

Index strus::getFirstAllMatchDocno(
		std::vector<Reference< PostingIteratorInterface> >& args,
		Index docno)
{
	if (args.empty()) return 0;
	
	Index docno_iter = docno;
	for (;;)
	{
		std::vector<Reference< PostingIteratorInterface> >::iterator
			ai = args.begin(), ae = args.end();
	
		docno_iter = (*ai)->skipDoc( docno_iter);
		if (docno_iter == 0)
		{
			return 0;
		}
		for (++ai; ai != ae; ++ai)
		{
			Index docno_next = (*ai)->skipDoc( docno_iter);
			if (docno_next == 0)
			{
				return 0;
			}
			if (docno_next != docno_iter)
			{
				docno_iter = docno_next;
				break;
			}
		}
		if (ai == ae)
		{
			return docno_iter;
		}
	}
}

Index strus::getFirstAllMatchDocnoSubset(
		std::vector<Reference< PostingIteratorInterface> >& args,
		Index docno,
		std::size_t cardinality)
{
	if (args.empty()) return 0;

	Index docno_iter = docno;
	for (;;)
	{
		std::vector<Reference< PostingIteratorInterface> >::iterator
			ai = args.begin(), ae = args.end();

		std::size_t nof_matches = 0;
		Index match_docno = 0;
	AGAIN:
		for (; ai != ae; ++ai)
		{
			Index docno_next = (*ai)->skipDoc( docno_iter);
			if (docno_next)
			{
				if (match_docno)
				{
					if (match_docno == docno_next)
					{
						++nof_matches;
					}
					else if (match_docno > docno_next)
					{
						match_docno = docno_next;
						nof_matches = 0;
						ai = args.begin();
						goto AGAIN;
					}
					else
					{
						continue;
					}
				}
				else
				{
					match_docno = docno_next;
					nof_matches = 1;
				}
			}
		}
		if (nof_matches >= cardinality && match_docno)
		{
			return match_docno;
		}
		else if (match_docno)
		{
			docno_iter = match_docno+1;
		}
		else
		{
			break;
		}
	}
	return 0;
}

void strus::encodeInteger( std::string& buf, int val)
{
	std::ostringstream num;
	num << val;
	buf.append( num.str());
}

