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
#include "summarizerMatchPhrase.hpp"
#include "postingIteratorLink.hpp"
#include "strus/arithmeticVariant.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/constants.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/utils.hpp"
#include <cstdlib>

using namespace strus;

SummarizerFunctionContextMatchPhrase::SummarizerFunctionContextMatchPhrase(
		const StorageClientInterface* storage_,
		const QueryProcessorInterface* processor_,
		const std::string& type_,
		unsigned int nofsummaries_,
		unsigned int summarylen_,
		unsigned int structseeklen_,
		const std::pair<std::string,std::string>& matchmark_,
		ErrorBufferInterface* errorhnd_)
	:m_storage(storage_)
	,m_processor(processor_)
	,m_forwardindex(storage_->createForwardIterator( type_))
	,m_type(type_)
	,m_nofsummaries(nofsummaries_)
	,m_summarylen(summarylen_)
	,m_structseeklen(structseeklen_)
	,m_matchmark(matchmark_)
	,m_nofCollectionDocuments((float)storage_->nofDocumentsInserted())
	,m_itr()
	,m_phrasestruct(0)
	,m_structop()
	,m_init_complete(false)
	,m_errorhnd(errorhnd_)
{
	if (!m_forwardindex.get()) throw strus::runtime_error(_TXT("error creating forward index iterator"));
}

void SummarizerFunctionContextMatchPhrase::addSummarizationFeature(
		const std::string& name,
		PostingIteratorInterface* itr,
		const std::vector<SummarizationVariable>&,
		float /*weight*/,
		const TermStatistics&)
{
	try
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
			m_errorhnd->report( _TXT("unknown '%s' summarization function parameter '%s'"), "MatchPhrase", name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding feature to '%s' summarizer: %s"), "matchphrase", *m_errorhnd);
}

SummarizerFunctionContextMatchPhrase::~SummarizerFunctionContextMatchPhrase()
{}

std::vector<SummarizerFunctionContextInterface::SummaryElement>
	SummarizerFunctionContextMatchPhrase::getSummary( const Index& docno)
{
	try
	{
		bool has_struct = false;
		if (!m_init_complete)
		{
			// Create the end of structure delimiter iterator:
			if (m_structelem.size() > 1)
			{
				const PostingJoinOperatorInterface*
					join = m_processor->getPostingJoinOperator( Constants::operator_set_union());
				if (!join)
				{
					m_errorhnd->explain(_TXT("error creating struct element iterator: %s"));
				}
				m_structop.reset( join->createResultIterator( m_structelem, 0, 0));
				if (!m_structop.get())
				{
					m_errorhnd->explain(_TXT("error creating struct element iterator: %s"));
				}
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
		std::vector<SummarizerFunctionContextInterface::SummaryElement> rt;
		m_forwardindex->skipDoc( docno);
		if (m_phrasestruct)
		{
			has_struct = (docno == m_phrasestruct->skipDoc( docno));
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
			Index pos = wi->m_posno;
			if (has_struct)
			{
				Index ph = m_phrasestruct->skipPos( pos<=(Index)m_structseeklen?1:(pos-m_structseeklen));
				if (ph && ph < pos)
				{
					pos = ph+1;
				}
				else
				{
					pos -= (m_structseeklen/2);
					if (pos <= 0) pos = 1;
				}
			}
			else
			{
				pos -= m_structseeklen;
				if (pos <= 0) pos = 1;
			}
			if (lastpos && pos < lastpos)
			{
				pos = lastpos+1;
				if (pos > wi->m_posno) continue;
			}
			if (has_struct)
			{
				lastpos = wi->m_posno + wi->m_size;
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
				lastpos = wi->m_posno + wi->m_size + (m_structseeklen/2);
			}
			bool containsMatch = ((unsigned int)(wi->m_posno + wi->m_size) <= (unsigned int)(lastpos) && wi->m_posno >= pos);
			if (containsMatch)
			{
				enum {MatchBefore,MatchAfter,MatchDone} matchState = MatchBefore;
				for (; pos <= lastpos; ++pos)
				{
					if (m_forwardindex->skipPos(pos) == pos)
					{
						if (!phrase.empty()) phrase.push_back(' ');
						switch (matchState)
						{
							case MatchBefore:
								if (pos == wi->skipPos(pos))
								{
									phrase.append( m_matchmark.first);
									matchState = MatchAfter;
								}
								break;
							case MatchAfter:
								if (pos != wi->skipPos(pos))
								{
									phrase.append( m_matchmark.second);
									matchState = MatchBefore;
								}
								break;
							case MatchDone:
								break;
						}
						phrase.append( m_forwardindex->fetch());
					}
				}
				if (matchState == MatchAfter)
				{
					phrase.append( m_matchmark.second);
					matchState = MatchDone;
				}
				rt.push_back( SummaryElement( phrase, 1.0));
			}
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summary: %s"), "matchphrase", *m_errorhnd, std::vector<SummarizerFunctionContextInterface::SummaryElement>());
}


void SummarizerFunctionInstanceMatchPhrase::addStringParameter( const std::string& name, const std::string& value)
{
	try
	{
		if (utils::caseInsensitiveEquals( name, "match") || utils::caseInsensitiveEquals( name, "struct"))
		{
			m_errorhnd->report( _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as string"), name.c_str(), "matchvariables");
		}
		else if (utils::caseInsensitiveEquals( name, "type"))
		{
			m_type = value;
		}
		else if (utils::caseInsensitiveEquals( name, "mark"))
		{
			char const* mid = std::strchr( value.c_str(), '$');
			while (mid[1] == '$')
			{
				mid = std::strchr( mid+2, '$');
			}
			if (mid)
			{
				m_matchmark.first = std::string( value.c_str(), mid - value.c_str());
				m_matchmark.second = std::string( mid+1);
			}
			else
			{
				if (m_matchmark.first.size())
				{
					m_matchmark.second = value;
				}
				else
				{
					m_matchmark.first = value;
				}
			}
		}
		else if (utils::caseInsensitiveEquals( name, "nof"))
		{
			m_errorhnd->report( _TXT("no string value expected for parameter '%s' in summarization function '%s'"), name.c_str(), "MatchPhrase");
		}
		else if (utils::caseInsensitiveEquals( name, "len"))
		{
			m_errorhnd->report( _TXT("no string value expected for parameter '%s' in summarization function '%s'"), name.c_str(), "MatchPhrase");
		}
		else if (utils::caseInsensitiveEquals( name, "structseek"))
		{
			m_errorhnd->report( _TXT("no string value expected for parameter '%s' in summarization function '%s'"), name.c_str(), "MatchPhrase");
		}
		else
		{
			m_errorhnd->report( _TXT("unknown '%s' summarization function parameter '%s'"), "MatchPhrase", name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding string parameter to '%s' summarizer: %s"), "matchphrase", *m_errorhnd);
}

void SummarizerFunctionInstanceMatchPhrase::addNumericParameter( const std::string& name, const ArithmeticVariant& value)
{
	if (utils::caseInsensitiveEquals( name, "match") || utils::caseInsensitiveEquals( name, "struct"))
	{
		m_errorhnd->report( _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as numeric value"), name.c_str(), "matchphrase");
	}
	else if (utils::caseInsensitiveEquals( name, "nof"))
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
		m_errorhnd->report( _TXT("no numeric value expected for parameter '%s' in summarization function '%s'"), name.c_str(), "MatchPhrase");
	}
	else if (utils::caseInsensitiveEquals( name, "mark"))
	{
		m_errorhnd->report( _TXT("no numeric value expected for parameter '%s' in summarization function '%s'"), name.c_str(), "MatchPhrase");
	}
	else
	{
		m_errorhnd->report( _TXT("unknown '%s' summarization function parameter '%s'"), "MatchPhrase", name.c_str());
	}
}

SummarizerFunctionContextInterface* SummarizerFunctionInstanceMatchPhrase::createFunctionContext(
		const StorageClientInterface* storage,
		MetaDataReaderInterface*,
		const GlobalStatistics&) const
{
	if (m_type.empty())
	{
		m_errorhnd->report( _TXT( "emtpy term type definition (parameter 'type') in match phrase summarizer configuration"));
	}
	try
	{
		return new SummarizerFunctionContextMatchPhrase(
				storage, m_processor, m_type, m_nofsummaries, m_summarylen, m_structseeklen, m_matchmark, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' summarizer: %s"), "matchphrase", *m_errorhnd, 0);
}

std::string SummarizerFunctionInstanceMatchPhrase::tostring() const
{
	try
	{
		std::ostringstream rt;
		rt << "type='" << m_type 
			<< "', nof summaries=" << m_nofsummaries
			<< ", summarylen=" << m_summarylen;
		return rt.str();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error mapping '%s' summarizer to string: %s"), "matchphrase", *m_errorhnd, std::string());
}


SummarizerFunctionInstanceInterface* SummarizerFunctionMatchPhrase::createInstance(
		const QueryProcessorInterface* processor) const
{
	try
	{
		return new SummarizerFunctionInstanceMatchPhrase( processor, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of '%s' summarizer: %s"), "matchphrase", *m_errorhnd, 0);
}

SummarizerFunctionInterface::Description SummarizerFunctionMatchPhrase::getDescription() const
{
	try
	{
		Description rt( _TXT("Get best matching phrases delimited by the structure postings"));
		rt( Description::Param::Feature, "match", _TXT( "defines the features to weight"));
		rt( Description::Param::Feature, "struct", _TXT( "defines the delimiter for structures"));
		rt( Description::Param::String, "type", _TXT( "the forward index type of the result phrase elements"));
		rt( Description::Param::Numeric, "nof", _TXT( "the maximum number of phrases extracted"));
		rt( Description::Param::Numeric, "len", _TXT( "restrict the lenght of the matching phrases"));
		rt( Description::Param::Numeric, "structseek", _TXT( "defines +/- the area where to search for structure delimiters"));
		rt( Description::Param::String, "mark", _TXT( "specifies a start and end tag (separated by '$') for highlighting matches in the resulting phrases"));
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating summarizer function description for '%s': %s"), "matchphrase", *m_errorhnd, SummarizerFunctionInterface::Description());
}

