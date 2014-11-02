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
		unsigned int maxlen_,
		unsigned int summarylen_,
		std::size_t nofitrs_,
		const IteratorInterface** itrs_,
		const IteratorInterface* phrasestruct_)
	:m_storage(storage_)
	,m_forwardindex( storage_->createForwardIndexViewer( termtype_))
	,m_termtype(termtype_)
	,m_maxlen(maxlen_)
	,m_summarylen(summarylen_)
{
	for (std::size_t ii=0; ii<nofitrs_; ++ii)
	{
		if (itrs_[ii])
		{
			m_itr.push_back( itrs_[ii]->copy());
		}
	}
	if (phrasestruct_)
	{
		m_phrasestruct.reset( phrasestruct_->copy());
	}
}

SummarizerMatchPhrase::~SummarizerMatchPhrase()
{
	if (m_forwardindex) delete m_forwardindex;
}

static Index getStartPos( Index curpos, unsigned int maxlen, IteratorInterface* phrasestruct, bool& found)
{
	found = true;
	Index rangepos = (curpos > maxlen) ? (curpos-maxlen):1;
	Index prevpos = phrasestruct?phrasestruct->skipPos( rangepos):0;
	if (!prevpos || prevpos > curpos)
	{
		prevpos = rangepos;
		if (rangepos > 1)
		{
			found = false;
		}
	}
	else if (phrasestruct) for (;;)
	{
		Index midpos = phrasestruct->skipPos( prevpos+1);
		if (!midpos || midpos > curpos) break;
		prevpos = midpos;
	}
	return prevpos;
	
}

static Index getEndPos( Index curpos, unsigned int maxlen, IteratorInterface* phrasestruct, bool& found)
{
	found = true;
	Index endpos = phrasestruct?phrasestruct->skipPos( curpos):0;
	if (!endpos || endpos - curpos > maxlen)
	{
		found = false;
		endpos = curpos + maxlen;
	}
	return endpos;
}

static std::string
	summaryElement(
		const Index& curpos,
		IteratorInterface* phrasestruct,
		ForwardIndexViewerInterface& forwardindex,
		unsigned int maxlen,
		unsigned int& length)
{
	bool start_found = true;
	Index startpos = getStartPos( curpos, maxlen, phrasestruct, start_found);
	bool end_found = true;
	Index endpos = getEndPos( curpos, maxlen, phrasestruct, end_found);

	length = 0;
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
	return phrase;
}

static void getSummary_(
		std::vector<std::string>& res,
		const Index& docno,
		const std::vector<IteratorReference>& itr,
		const IteratorReference& phrasestruct,
		ForwardIndexViewerInterface& forwardindex,
		unsigned int maxlen,
		unsigned int maxsummarylen)
{
	forwardindex.initDoc( docno);
	Index curpos = 0;

	std::vector<IteratorReference>::const_iterator
		ii = itr.begin(), ie = itr.end();
	unsigned int summarylen = 0;

	for (; ii != ie && summarylen < maxsummarylen; ++ii)
	{
		if (*ii && docno==(*ii)->skipDoc( docno))
		{
			while (0!=(curpos=(*ii)->skipPos( curpos)))
			{
				unsigned int length = 0;
				res.push_back(
					summaryElement( 
						curpos, phrasestruct.get(),
						forwardindex, maxlen, length));
				summarylen += length;
				curpos += length + 1;
	
				if (summarylen >= maxsummarylen)
				{
					break;
				}
			}
		}
	}
}


std::vector<std::string>
	SummarizerMatchPhrase::getSummary( const Index& docno)
{
	std::vector<std::string> rt;
	getSummary_(
		rt, docno, m_itr, m_phrasestruct,
		*m_forwardindex, m_maxlen, m_summarylen);
	return rt;
}


