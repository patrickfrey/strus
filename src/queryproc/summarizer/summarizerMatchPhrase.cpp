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
#include "postingIteratorLink.hpp"
#include "strus/arithmeticVariant.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/constants.hpp"
#include "private/internationalization.hpp"
#include "private/utils.hpp"
#include <cstdlib>

using namespace strus;

void SummarizerFunctionInstanceMatchPhrase::addParameter( const std::string& name, const std::string& value)
{
	if (utils::caseInsensitiveEquals( name, "termtype"))
	{
		m_termtype = value;
	}
	else if (utils::caseInsensitiveEquals( name, "phraselen"))
	{
		throw strus::runtime_error( _TXT("no string value expected for parameter '%s' in summarization function '%s'"), name.c_str(), "MatchPhrase");
	}
	else if (utils::caseInsensitiveEquals( name, "sumlen"))
	{
		throw strus::runtime_error( _TXT("no string value expected for parameter '%s' in summarization function '%s'"), name.c_str(), "MatchPhrase");
	}
	else
	{
		throw strus::runtime_error( _TXT("unknown '%s' summarization function parameter '%s'"), "MatchPhrase", name.c_str());
	}
}

void SummarizerFunctionInstanceMatchPhrase::addParameter( const std::string& name, const ArithmeticVariant& value)
{
	if (utils::caseInsensitiveEquals( name, "phraselen"))
	{
		m_maxlen = (unsigned int)value;
	}
	else if (utils::caseInsensitiveEquals( name, "sumlen"))
	{
		m_sumlen = (unsigned int)value;
	}
	else if (utils::caseInsensitiveEquals( name, "termtype"))
	{
		throw strus::runtime_error( _TXT("no numeric value expected for parameter '%s' in summarization function '%s'"), name.c_str(), "MatchPhrase");
	}
	else
	{
		throw strus::runtime_error( _TXT("unknown '%s' summarization function parameter '%s'"), "MatchPhrase", name.c_str());
	}
}


SummarizerClosureMatchPhrase::SummarizerClosureMatchPhrase(
		const StorageClientInterface* storage_,
		const QueryProcessorInterface* processor_,
		const std::string& termtype_,
		unsigned int maxlen_,
		unsigned int summarylen_)
	:m_storage(storage_)
	,m_processor(processor_)
	,m_forwardindex(storage_->createForwardIterator( termtype_))
	,m_termtype(termtype_)
	,m_maxlen(maxlen_)
	,m_summarylen(summarylen_)
	,m_itr()
	,m_phrasestruct(0)
	,m_structop()
	,m_init_complete(false)
{}

void SummarizerClosureMatchPhrase::addSummarizationFeature(
		const std::string& name,
		PostingIteratorInterface* itr,
		const std::vector<SummarizationVariable>&)
{
	if (utils::caseInsensitiveEquals( name, "struct"))
	{
		m_phrasestruct = itr;
		Reference<PostingIteratorInterface> ref( new PostingIteratorLink( itr));
		m_structelem.push_back( ref);
	}
	else if (utils::caseInsensitiveEquals( name, "match"))
	{
		m_itr.push_back( itr);
	}
	else
	{
		throw strus::runtime_error( _TXT("unknown '%s' summarization function parameter '%s'"), "MatchPhrase", name.c_str());
	}
}

SummarizerClosureMatchPhrase::~SummarizerClosureMatchPhrase()
{}

static Index getStartPos( Index curpos, unsigned int maxlen, PostingIteratorInterface* phrasestruct, bool& found)
{
	found = true;
	Index rangepos = ((unsigned int)curpos > maxlen) ? ((unsigned int)curpos-maxlen):1;
	Index prevpos = phrasestruct?phrasestruct->skipPos( rangepos):0;
	if (!prevpos || prevpos > curpos)
	{
		prevpos = rangepos;
		if (rangepos > 1)
		{
			found = false;
		}
	}
	if (phrasestruct) for (;;)
	{
		Index midpos = phrasestruct->skipPos( prevpos+1);
		if (!midpos || midpos > curpos) break;
		prevpos = midpos;
	}
	return prevpos;
	
}

static Index getEndPos( Index curpos, unsigned int maxlen, PostingIteratorInterface* phrasestruct, bool& found)
{
	found = true;
	Index endpos = phrasestruct?phrasestruct->skipPos( curpos):0;
	if (!endpos || endpos - curpos > (Index)maxlen)
	{
		found = false;
		endpos = curpos + maxlen;
	}
	return endpos;
}

static SummarizerClosureInterface::SummaryElement
	summaryElement(
		const Index& curpos,
		PostingIteratorInterface* phrasestruct,
		ForwardIteratorInterface& forwardindex,
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
	for (;pp < endpos; ++pp)
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
	return SummarizerClosureInterface::SummaryElement( phrase, 1.0);
}

static void getSummary_(
		std::vector<SummarizerClosureInterface::SummaryElement>& res,
		const Index& docno,
		std::vector<PostingIteratorInterface*> itr,
		PostingIteratorInterface* phrasestruct,
		ForwardIteratorInterface& forwardindex,
		unsigned int maxlen,
		unsigned int maxsummarylen)
{
	forwardindex.skipDoc( docno);
	if (phrasestruct)
	{
		phrasestruct->skipDoc( docno);
	}
	Index curpos = 0;
	Index nextpos = 0;

	std::vector<PostingIteratorInterface*>::const_iterator
		ii = itr.begin(), ie = itr.end();
	unsigned int summarylen = 0;

	for (; ii != ie && summarylen < maxsummarylen; ++ii)
	{
		if (docno==(*ii)->skipDoc( docno))
		{
			while (0!=(nextpos=(*ii)->skipPos( curpos)))
			{
				unsigned int length = 0;
				res.push_back(
					summaryElement( 
						nextpos, phrasestruct,
						forwardindex, maxlen, length));
				summarylen += length;
				curpos = nextpos + length + 1;
	
				if (summarylen >= maxsummarylen)
				{
					break;
				}
			}
		}
	}
}


std::vector<SummarizerClosureInterface::SummaryElement>
	SummarizerClosureMatchPhrase::getSummary( const Index& docno)
{
	if (!m_init_complete)
	{
		if (m_structelem.size() > 1)
		{
			const PostingJoinOperatorInterface*
				join = m_processor->getPostingJoinOperator( Constants::operator_set_union());
	
			m_structop.reset( join->createResultIterator( m_structelem, 0));
			m_phrasestruct = m_structop.get();
		}
		m_init_complete = true;
	}
	std::vector<SummarizerClosureInterface::SummaryElement> rt;
	getSummary_(
		rt, docno, m_itr, m_phrasestruct,
		*m_forwardindex.get(), m_maxlen, m_summarylen);
	return rt;
}


