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

void SummarizerFunctionInstanceMatchPhrase::addStringParameter( const std::string& name, const std::string& value)
{
	if (utils::caseInsensitiveEquals( name, "type"))
	{
		m_type = value;
	}
	else if (utils::caseInsensitiveEquals( name, "nof"))
	{
		throw strus::runtime_error( _TXT("no string value expected for parameter '%s' in summarization function '%s'"), name.c_str(), "MatchPhrase");
	}
	else if (utils::caseInsensitiveEquals( name, "len"))
	{
		throw strus::runtime_error( _TXT("no string value expected for parameter '%s' in summarization function '%s'"), name.c_str(), "MatchPhrase");
	}
	else if (utils::caseInsensitiveEquals( name, "structseek"))
	{
		throw strus::runtime_error( _TXT("no string value expected for parameter '%s' in summarization function '%s'"), name.c_str(), "MatchPhrase");
	}
	else
	{
		throw strus::runtime_error( _TXT("unknown '%s' summarization function parameter '%s'"), "MatchPhrase", name.c_str());
	}
}

void SummarizerFunctionInstanceMatchPhrase::addNumericParameter( const std::string& name, const ArithmeticVariant& value)
{
	if (utils::caseInsensitiveEquals( name, "nof"))
	{
		m_nofsummaries = (unsigned int)value;
	}
	else if (utils::caseInsensitiveEquals( name, "len"))
	{
		m_summarylen = (unsigned int)value;
	}
	else if (utils::caseInsensitiveEquals( name, "structseek"))
	{
		m_structseeklen = (unsigned int)value;
	}
	else if (utils::caseInsensitiveEquals( name, "type"))
	{
		throw strus::runtime_error( _TXT("no numeric value expected for parameter '%s' in summarization function '%s'"), name.c_str(), "MatchPhrase");
	}
	else
	{
		throw strus::runtime_error( _TXT("unknown '%s' summarization function parameter '%s'"), "MatchPhrase", name.c_str());
	}
}


SummarizerExecutionContextMatchPhrase::SummarizerExecutionContextMatchPhrase(
		const StorageClientInterface* storage_,
		const QueryProcessorInterface* processor_,
		const std::string& type_,
		unsigned int nofsummaries_,
		unsigned int summarylen_,
		unsigned int structseeklen_)
	:m_storage(storage_)
	,m_processor(processor_)
	,m_forwardindex(storage_->createForwardIterator( type_))
	,m_type(type_)
	,m_nofsummaries(nofsummaries_)
	,m_summarylen(summarylen_)
	,m_structseeklen(structseeklen_)
	,m_nofCollectionDocuments((float)storage_->globalNofDocumentsInserted())
	,m_itr()
	,m_phrasestruct(0)
	,m_structop()
	,m_init_complete(false)
{}

void SummarizerExecutionContextMatchPhrase::addSummarizationFeature(
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
		float df = (float)itr->documentFrequency();
		float idf = logf( (m_nofCollectionDocuments - df + 0.5) / (df + 0.5));
		m_weights.push_back( idf);
	}
	else
	{
		throw strus::runtime_error( _TXT("unknown '%s' summarization function parameter '%s'"), "MatchPhrase", name.c_str());
	}
}

SummarizerExecutionContextMatchPhrase::~SummarizerExecutionContextMatchPhrase()
{}

std::vector<SummarizerExecutionContextInterface::SummaryElement>
	SummarizerExecutionContextMatchPhrase::getSummary( const Index& docno)
{
	if (!m_init_complete)
	{
		// Create the end of structure delimiter iterator:
		if (m_structelem.size() > 1)
		{
			const PostingJoinOperatorInterface*
				join = m_processor->getPostingJoinOperator( Constants::operator_set_union());
	
			m_structop.reset( join->createResultIterator( m_structelem, 0));
			m_phrasestruct = m_structop.get();
		}

		m_init_complete = true;
	}
	// Create the sliding window for fetching the best matches:
	std::set<SlidingMatchWindow::Match> matchset;
	std::vector<PostingIteratorInterface*>::const_iterator
		ii = m_itr.begin(), ei = m_itr.end();
	for (unsigned int iidx=0; ii != ei; ++ii,++iidx)
	{
		if (docno==(*ii)->skipDoc( docno))
		{
			Index firstpos = (*ii)->skipPos(0);
			if (firstpos)
			{
				matchset.insert( SlidingMatchWindow::Match( firstpos, m_weights[iidx], iidx));
			}
		}
	}
	SlidingMatchWindow slidingMatchWindow( m_summarylen, m_nofsummaries*4, matchset);

	// Initialize the forward index and the structure elements:
	std::vector<SummarizerExecutionContextInterface::SummaryElement> rt;
	m_forwardindex->skipDoc( docno);
	if (m_phrasestruct)
	{
		m_phrasestruct->skipDoc( docno);
	}

	// Calculate the best matching passages with help of the sliding window:
	while (!slidingMatchWindow.finished())
	{
		unsigned int iidx = slidingMatchWindow.firstPostingIdx();
		Index pos = m_itr[iidx]->skipPos( m_itr[iidx]->posno()+1);
		slidingMatchWindow.push( iidx, pos);
	}

	// Build the result (best passages):
	std::vector<SlidingMatchWindow::Window> war = slidingMatchWindow.getResult();
	std::vector<SlidingMatchWindow::Window>::const_iterator wi = war.begin(), we = war.end();
	Index lastpos = 0;
	for (; wi != we && rt.size() < m_nofsummaries; ++wi)
	{
		std::string phrase;
		Index pos = wi->pos;
		if (m_phrasestruct)
		{
			Index ph = m_phrasestruct->skipPos( pos<=(Index)m_structseeklen?1:(pos-m_structseeklen));
			if (ph && ph < pos)
			{
				pos = ph;
			}
		}
		else
		{
			pos -= (m_structseeklen/2);
			if (pos <= 0) pos = 1;
		}
		if (lastpos && pos < lastpos)
		{
			pos = lastpos+1;
			if (pos > wi->pos) continue;
		}
		if (m_phrasestruct)
		{
			lastpos = wi->pos + wi->size;
			Index lp = m_phrasestruct->skipPos( lastpos);
			{
				if (lp && (Index)(lastpos + m_structseeklen) > lp)
				{
					lastpos = lp;
				}
				else
				{
					lastpos += (m_structseeklen/2);
				}
			}
		}
		else
		{
			lastpos = wi->pos + wi->size + (m_structseeklen/2);
		}
		bool containsMatch = ((unsigned int)(wi->pos + wi->size) <= (unsigned int)(lastpos) && wi->pos >= pos);
		if (containsMatch)
		{
			for (; pos <= lastpos; ++pos)
			{
				if (m_forwardindex->skipPos(pos) == pos)
				{
					if (!phrase.empty()) phrase.push_back(' ');
					phrase.append( m_forwardindex->fetch());
				}
			}
			rt.push_back( SummaryElement( phrase, 1.0));
		}
	}
	return rt;
}


