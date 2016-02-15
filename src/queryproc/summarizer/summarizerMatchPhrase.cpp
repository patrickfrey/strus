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
#include "proximityWeightAccumulator.hpp"
#include "positionWindow.hpp"
#include "postingIteratorLink.hpp"
#include "strus/arithmeticVariant.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/constants.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/utils.hpp"
#include <cstdlib>

using namespace strus;

SummarizerFunctionContextMatchPhrase::SummarizerFunctionContextMatchPhrase(
		const StorageClientInterface* storage_,
		const MetaDataReaderInterface* metadata_,
		const QueryProcessorInterface* processor_,
		const std::string& type_,
		unsigned int sentencesize_,
		unsigned int windowsize_,
		unsigned int cardinality_,
		double nofCollectionDocuments_,
		const std::string& attribute_header_maxpos_,
		const std::pair<std::string,std::string>& matchmark_,
		ErrorBufferInterface* errorhnd_)
	:m_storage(storage_)
	,m_metadata(metadata_)
	,m_processor(processor_)
	,m_forwardindex(storage_->createForwardIterator( type_))
	,m_type(type_)
	,m_sentencesize(sentencesize_)
	,m_windowsize(windowsize_)
	,m_cardinality(cardinality_)
	,m_nofCollectionDocuments(nofCollectionDocuments_)
	,m_metadata_header_maxpos(attribute_header_maxpos_.empty()?-1:metadata_->elementHandle( attribute_header_maxpos_))
	,m_matchmark(matchmark_)
	,m_itrarsize(0)
	,m_structarsize(0)
	,m_initialized(false)
	,m_errorhnd(errorhnd_)
{
	if (!m_forwardindex.get()) throw strus::runtime_error(_TXT("error creating forward index iterator"));
}

void SummarizerFunctionContextMatchPhrase::addSummarizationFeature(
		const std::string& name,
		PostingIteratorInterface* itr,
		const std::vector<SummarizationVariable>&,
		float weight,
		const TermStatistics& termstats)
{
	try
	{
		if (utils::caseInsensitiveEquals( name, "struct"))
		{
			if (m_structarsize + m_structarsize > MaxNofArguments) throw strus::runtime_error( _TXT("number of structure features out of range"));
			m_structar[ m_structarsize + m_paraarsize] = m_structar[ m_structarsize];
			m_structar[ m_structarsize++] = itr;
		}
		else if (utils::caseInsensitiveEquals( name, "para"))
		{
			if (m_paraarsize + m_structarsize > MaxNofArguments) throw strus::runtime_error( _TXT("number of structure features out of range"));
			m_structar[ m_structarsize + m_paraarsize] = itr;
			m_paraar[ m_paraarsize++] = itr;
		}
		else if (utils::caseInsensitiveEquals( name, "match"))
		{
			if (m_itrarsize > MaxNofArguments) throw strus::runtime_error( _TXT("number of weighting features out of range"));

			double df = termstats.documentFrequency()>=0?termstats.documentFrequency():(GlobalCounter)itr->documentFrequency();
			double idf = logl( (m_nofCollectionDocuments - df + 0.5) / (df + 0.5));
			m_itrar[ m_itrarsize++] = itr;
			m_idfar.add( idf * weight);
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

static void callSkipDoc( const Index& docno, PostingIteratorInterface** ar, std::size_t arsize)
{
	for (std::size_t ai=0; ai < arsize; ++ai)
	{
		ar[ ai]->skipDoc( docno);
	}
}

std::vector<SummarizerFunctionContextInterface::SummaryElement>
	SummarizerFunctionContextMatchPhrase::getSummary( const Index& docno)
{
	try
	{
		std::vector<SummarizerFunctionContextInterface::SummaryElement> rt;
		if (!m_initialized)
		{
			// initialize proportional ff increment weights
			m_weightincr.init( m_itrarsize);
			ProximityWeightAccumulator::proportionalAssignment( m_weightincr, 1.0, 0.3, m_idfar);
			m_initialized = true;
		}
		// Init document iterators:
		callSkipDoc( docno, m_itrar, m_itrarsize);
		callSkipDoc( docno, m_structar, m_structarsize);
		callSkipDoc( docno, m_paraar, m_paraarsize);

		// Define search start position:
		Index firstpos = 0;
		if (m_metadata_header_maxpos>=0)
		{
			ArithmeticVariant firstposval = m_metadata->getValue( m_metadata_header_maxpos);
			firstpos = firstposval.toint()+1;
		}
		// Define best match to find:
		struct
		{
			double weight;
			Index pos;
			Index span;
		} candidate = {0.0,firstpos,0};

		PositionWindow poswin( m_itrar, m_itrarsize, m_windowsize, m_cardinality,
					firstpos, PositionWindow::MaxWin);
		bool more = poswin.first();
		for (;more; more = poswin.next())
		{
			// Check if window is overlapping a paragraph. In this case to not use it for summary:
			std::size_t gi=0;
			for (; gi<m_paraarsize; ++gi)
			{
				Index pos = poswin.pos();
				Index nextpara = m_paraar[gi]->skipPos( pos+1);
				if (nextpara - pos < (Index)poswin.span()) break;
			}
			if (gi < m_paraarsize) continue;

			// Calculate the weight of the current window:
			ProximityWeightAccumulator::WeightArray weightar( m_itrarsize);

			const std::size_t* window = poswin.window();
			std::size_t windowsize = poswin.size();

			ProximityWeightAccumulator::weight_same_sentence(
				weightar, 0.3, m_weightincr, window, windowsize, m_itrar, m_itrarsize, m_structar, m_structarsize);
		
			ProximityWeightAccumulator::weight_imm_follow(
				weightar, 0.4, m_weightincr, window, windowsize, m_itrar, m_itrarsize);
		
			ProximityWeightAccumulator::weight_invdist(
				weightar, 0.3, m_weightincr, window, windowsize, m_itrar, m_itrarsize);

			if (poswin.pos() < 1000)
			{
				ProximityWeightAccumulator::weight_invpos(
					weightar, 0.5, m_weightincr, firstpos, m_itrar, m_itrarsize);
			}
			weightar.multiply( m_idfar);
			double weight = weightar.sum();

			// Select the best window:
			if (weight > candidate.weight)
			{
				candidate.weight = weight;
				candidate.pos = poswin.pos();
				candidate.span = poswin.span();
			}
		}
		struct
		{
			Index start;
			Index span;
			bool defined_start;
			bool defined_end;
		} abstract = {0,0,false,false};

		// Find the start of the abstract as start of the preceeding structure or paragraph marker:
		std::size_t ti=0,te=m_paraarsize+m_structarsize;
		Index astart = (Index)m_sentencesize > candidate.pos ? (candidate.pos - (Index)m_sentencesize):firstpos;
		for (std::size_t ti=0; ti<te && astart < candidate.pos; ++ti)
		{
			Index pos = m_structar[ti]->skipPos( astart);
			while (pos > astart && pos <= candidate.pos)
			{
				abstract.defined_start = true;
				astart = pos;
				pos = m_structar[ti]->skipPos( astart+1);
			}
		}
		abstract.span += candidate.pos - astart;
		abstract.start = astart;

		// Find the end of the abstract, by scanning for the next sentence:
		if (abstract.span < (Index)m_sentencesize)
		{
			Index aspan = (Index)m_sentencesize;
			for (ti=0; ti<te; ++ti)
			{
				Index pos = m_structar[ti]->skipPos( abstract.start + abstract.span);
				if (pos && (pos - abstract.start) < aspan)
				{
					aspan = pos - abstract.start;
					abstract.defined_end = true;
				}
			}
			abstract.span = aspan;
		}

		// Create the highlighted result, if exists:
		if (abstract.span > 0)
		{
			// Create the array of positions to highlight:
			std::vector<Index> highlightpos;
			Index pi = abstract.start, pe = abstract.start + abstract.span;
			while (pi < pe)
			{
				Index minpos = 0;
				std::size_t ti=0, te=m_itrarsize;
				for (;ti<te; ++ti)
				{
					Index pos = m_itrar[ti]->skipPos( pi);
					if (pos && pos < pe)
					{
						if (!minpos || pos < minpos)
						{
							minpos = pos;
						}
					}
				}
				if (minpos)
				{
					highlightpos.push_back( minpos);
					pi = minpos+1;
				}
				else
				{
					break;
				}
			}

			// Build the phrase:
			std::string phrase;
			if (!abstract.defined_start)
			{
				phrase.append( " ... ");
			}
			pi = abstract.start, pe = abstract.start + abstract.span;
			if (pi > pe || (unsigned int)(pe - pi) > 2*m_sentencesize+10)
			{
				throw strus::runtime_error(_TXT("internal: got illegal summary (%u:%u)"), (unsigned int)pi, (unsigned int)pe);
			}
			std::size_t hi = 0, he = highlightpos.size();
			for (; pi < pe; ++pi)
			{
				if (m_forwardindex->skipPos(pi) == pi)
				{
					if (!phrase.empty()) phrase.push_back(' ');
					for (; hi < he && pi < highlightpos[hi]; ++he){}
					if (hi < he && pi == highlightpos[hi])
					{
						phrase.append( m_matchmark.first);
						phrase.append( m_forwardindex->fetch());
						phrase.append( m_matchmark.second);
					}
					else
					{
						phrase.append( m_forwardindex->fetch());
					}
				}
			}
			rt.push_back( SummaryElement( phrase, 1.0));
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summary: %s"), "matchphrase", *m_errorhnd, std::vector<SummarizerFunctionContextInterface::SummaryElement>());
}


void SummarizerFunctionInstanceMatchPhrase::addStringParameter( const std::string& name, const std::string& value)
{
	try
	{
		if (utils::caseInsensitiveEquals( name, "match") || utils::caseInsensitiveEquals( name, "struct") || utils::caseInsensitiveEquals( name, "para"))
		{
			m_errorhnd->report( _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as string"), name.c_str(), "matchvariables");
		}
		else if (utils::caseInsensitiveEquals( name, "type"))
		{
			m_type = value;
		}
		else if (utils::caseInsensitiveEquals( name, "attribute_title_maxpos"))
		{
			m_attribute_title_maxpos = value;
		}
		else if (utils::caseInsensitiveEquals( name, "mark"))
		{
			if (!value.empty())
			{
				char sep = value[0];
				char const* mid = std::strchr( value.c_str()+1, sep);
				if (mid)
				{
					m_matchmark.first = std::string( value.c_str()+1, mid - value.c_str() - 1);
					m_matchmark.second = std::string( mid+1);
				}
				else
				{
					m_matchmark.first = value;
					m_matchmark.second = value;
				}
			}
		}
		else if (utils::caseInsensitiveEquals( name, "sentencesize"))
		{
			m_errorhnd->report( _TXT("no string value expected for parameter '%s' in summarization function '%s'"), name.c_str(), "MatchPhrase");
		}
		else if (utils::caseInsensitiveEquals( name, "windowsize"))
		{
			m_errorhnd->report( _TXT("no string value expected for parameter '%s' in summarization function '%s'"), name.c_str(), "MatchPhrase");
		}
		else if (utils::caseInsensitiveEquals( name, "cardinality"))
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
	if (utils::caseInsensitiveEquals( name, "match") || utils::caseInsensitiveEquals( name, "struct") || utils::caseInsensitiveEquals( name, "para"))
	{
		m_errorhnd->report( _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as numeric value"), name.c_str(), "matchphrase");
	}
	else if (utils::caseInsensitiveEquals( name, "sentencesize"))
	{
		m_sentencesize = (unsigned int)value;
	}
	else if (utils::caseInsensitiveEquals( name, "windowsize"))
	{
		m_windowsize = (unsigned int)value;
	}
	else if (utils::caseInsensitiveEquals( name, "cardinality"))
	{
		m_cardinality = (unsigned int)value;
	}
	else if (utils::caseInsensitiveEquals( name, "type")
		|| utils::caseInsensitiveEquals( name, "mark")
		|| utils::caseInsensitiveEquals( name, "attribute_title_maxpos"))
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
		MetaDataReaderInterface* metadata,
		const GlobalStatistics& stats) const
{
	if (m_type.empty())
	{
		m_errorhnd->report( _TXT( "emtpy term type definition (parameter 'type') in match phrase summarizer configuration"));
	}
	try
	{
		double nofCollectionDocuments = stats.nofDocumentsInserted()>=0?stats.nofDocumentsInserted():(GlobalCounter)storage->nofDocumentsInserted();
		return new SummarizerFunctionContextMatchPhrase(
				storage, metadata, m_processor, m_type, m_sentencesize, m_windowsize, m_cardinality,
				nofCollectionDocuments, m_attribute_title_maxpos, m_matchmark, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' summarizer: %s"), "matchphrase", *m_errorhnd, 0);
}

std::string SummarizerFunctionInstanceMatchPhrase::tostring() const
{
	try
	{
		std::ostringstream rt;
		rt << "type='" << m_type
			<< "', matchmark=(" << m_matchmark.first << "," << m_matchmark.second << ")"
			<< "', attribute_title_maxpos='" << m_attribute_title_maxpos
			<< "', sentencesize='" << m_sentencesize
			<< "', windowsize='" << m_windowsize
			<< "', cardinality='" << m_cardinality
			<< "'";
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
		rt( Description::Param::Feature, "match", _TXT( "defines the features to weight"), "");
		rt( Description::Param::Feature, "struct", _TXT( "defines the delimiter for structures"), "");
		rt( Description::Param::Feature, "para", _TXT( "defines the delimiter for paragraphs (summaries must not overlap paragraph borders)"), "");
		rt( Description::Param::String, "type", _TXT( "the forward index type of the result phrase elements"), "");
		rt( Description::Param::Numeric, "attribute_title_maxpos", _TXT( "the metadata attribute that specifies the last title element and thus the part of the content that is used for abstracting"), "1:");
		rt( Description::Param::Numeric, "sentencesize", _TXT( "restrict the maximum lenght of sentences in summaries"), "1:");
		rt( Description::Param::Numeric, "windowsize", _TXT( "maximum size of window used for identifying matches"), "1:");
		rt( Description::Param::Numeric, "cardinality", _TXT( "minimum number of features in a window"), "1:");
		rt( Description::Param::String, "mark", _TXT( "specifies the markers (first character of the value is the separator followed by the two parts separated by it) for highlighting matches in the resulting phrases"), "");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating summarizer function description for '%s': %s"), "matchphrase", *m_errorhnd, SummarizerFunctionInterface::Description());
}

