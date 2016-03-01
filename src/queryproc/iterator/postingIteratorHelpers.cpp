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
#include "private/bitOperations.hpp"
#include <sstream>

using namespace strus;

Index strus::getFirstAllMatchDocno(
		std::vector<Reference< PostingIteratorInterface> >& args,
		Index docno,
		bool allowEmpty)
{
	if (args.empty()) return 0;

	Index docno_iter = docno;
	std::vector<Reference< PostingIteratorInterface> >::iterator
		ae = args.end();
	for (;;)
	{
		std::vector<Reference< PostingIteratorInterface> >::iterator
			ai = args.begin();

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
				for (; ai != ae; ++ai)
				{
					if (docno_iter != (*ai)->skipDoc( docno_iter))
					{
						++docno_iter;
						continue;
					}
				}
			}
			return docno_iter;
		}
	}
}

Index strus::getFirstAllMatchDocnoSubset(
		std::vector<Reference< PostingIteratorInterface> >& args,
		Index docno,
		bool allowEmpty,
		std::size_t cardinality,
		uint64_t& candidate_set)
{
	if (args.empty()) return 0;
	if (args.size() > 64) throw std::runtime_error("number of arguments for getFirstAllMatchDocnoSubset out of range");

	Index docno_iter = docno;
	for (;;)
	{
		std::vector<Reference< PostingIteratorInterface> >::iterator
			ai = args.begin(), ae = args.end();

		std::size_t nof_matches = 0;
		Index match_docno = 0;
		candidate_set = 0;

		for (unsigned int aidx = 0; ai != ae; ++ai,++aidx)
		{
			Index docno_next = (*ai)->skipDocCandidate( docno_iter);
			if (docno_next)
			{
				if (match_docno)
				{
					if (match_docno == docno_next)
					{
						candidate_set |= (uint64_t)1<<(aidx);
						++nof_matches;
					}
					else if (match_docno > docno_next)
					{
						candidate_set = (uint64_t)1<<(aidx);
						match_docno = docno_next;
						nof_matches = 1;
					}
				}
				else
				{
					candidate_set = (uint64_t)1<<(aidx);
					match_docno = docno_next;
					nof_matches = 1;
				}
			}
		}
		if (nof_matches >= cardinality)
		{
			if (!allowEmpty)
			{
				unsigned int aidx = BitOperations::bitScanForward( candidate_set);
				while (aidx)
				{
					if (match_docno != args[aidx-1]->skipDoc( match_docno))
					{
						--nof_matches;
						if (nof_matches < cardinality) break;
					}
					candidate_set -= (uint64_t)1<<(aidx-1);
					aidx = BitOperations::bitScanForward( candidate_set);
				}
				if (nof_matches < cardinality)
				{
					docno_iter = match_docno+1;
					continue;
				}
			}
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

