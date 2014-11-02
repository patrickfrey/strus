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
#include "summarizerMatchPhrase.hpp"
#include "strus/iteratorInterface.hpp"
#include "strus/forwardIndexViewerInterface.hpp"
#include "strus/storageInterface.hpp"

using namespace strus;

SummarizerMatchPhrase::SummarizerMatchPhrase(
		StorageInterface* storage_,
		const std::string& termtype_,
		unsigned int maxlen_)
	:m_storage(storage_)
	,m_forwardindex( storage_->createForwardIndexViewer( termtype_))
	,m_termtype(termtype_)
	,m_maxlen(maxlen_)
{}

SummarizerMatchPhrase::~SummarizerMatchPhrase()
{
	if (m_forwardindex) delete m_forwardindex;
}

static Index getStartPos( Index curpos, unsigned int maxlen, IteratorInterface& markitr, bool& found)
{
	found = true;
	Index rangepos = (curpos > maxlen) ? (curpos-maxlen):1;
	Index prevpos = markitr.skipPos( rangepos);
	if (!prevpos || prevpos > curpos)
	{
		prevpos = rangepos;
		if (rangepos > 1)
		{
			found = false;
		}
	}
	else for (;;)
	{
		Index midpos = markitr.skipPos( prevpos+1);
		if (!midpos || midpos > curpos) break;
		prevpos = midpos;
	}
	return prevpos;
	
}

static Index getEndPos( Index curpos, unsigned int maxlen, IteratorInterface& markitr, bool& found)
{
	found = true;
	Index endpos = markitr.skipPos( curpos);
	if (endpos - curpos > maxlen)
	{
		found = false;
		endpos = curpos + maxlen;
	}
	return endpos;
}

static SummarizerInterface::SummaryElement
	summaryElement(
		const Index& curpos,
		IteratorInterface& markitr,
		ForwardIndexViewerInterface& forwardindex,
		unsigned int maxlen)
{
	bool start_found = true;
	Index startpos = getStartPos( curpos, maxlen, markitr, start_found);
	bool end_found = true;
	Index endpos = getEndPos( curpos, maxlen, markitr, end_found);

	unsigned int length = 0;
	std::string phrase;
	if (!start_found) phrase.append( "...");

	Index pp = startpos;
	for (;pp <= endpos; ++pp)
	{
		pp = forwardindex.skipPos(pp);
		if (pp)
		{
			if (!phrase.empty()) phrase.push_back(' ');
			phrase.append( forwardindex.fetch());
			++length;
		}
		else
		{
			break;
		}
	}
	if (!end_found) phrase.append( "...");
	return SummarizerInterface::SummaryElement( phrase, curpos, length);
}

static bool getSummary_(
		std::vector<SummarizerInterface::SummaryElement>& res,
		const Index& docno,
		const Index& pos,
		IteratorInterface& itr,
		IteratorInterface& markitr,
		ForwardIndexViewerInterface& forwardindex,
		unsigned int maxlen)
{
	bool rt = false;
	if (itr.skipDoc( docno) == docno)
	{
		forwardindex.initDoc( docno);
		Index curpos = itr.skipPos( pos);

		if (pos)
		{
			if (curpos)
			{
				res.push_back(
					summaryElement( 
						curpos, markitr,
						forwardindex, maxlen));
				rt = true;
			}
		}
		else
		{
			for (; curpos; curpos = itr.skipPos( pos+1))
			{
				res.push_back(
					summaryElement( 
						curpos, markitr,
						forwardindex, maxlen));
			}
		}
	}
	return rt;
}

bool
	SummarizerMatchPhrase::getSummary(
		std::vector<SummarizerInterface::SummaryElement>& res,
		const Index& docno,
		const Index& pos,
		IteratorInterface& itr,
		IteratorInterface& markitr)
{
	if (docno)
	{
		return getSummary_( res, docno, pos, itr, markitr, *m_forwardindex, m_maxlen);
	}
	else
	{
		Index dn = 0;
		bool rt = false;
		while (0!=(dn=itr.skipDoc( dn+1)))
		{
			rt |= getSummary_( res, dn, pos, itr, markitr, *m_forwardindex, m_maxlen);
		}
		return rt;
	}
}


