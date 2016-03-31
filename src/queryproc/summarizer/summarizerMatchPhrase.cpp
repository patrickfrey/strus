/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "summarizerMatchPhrase.hpp"
#include "proximityWeightAccumulator.hpp"
#include "positionWindow.hpp"
#include "postingIteratorLink.hpp"
#include "strus/numericVariant.hpp"
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
#undef STRUS_LOWLEVEL_DEBUG

SummarizerFunctionContextMatchPhrase::SummarizerFunctionContextMatchPhrase(
		const StorageClientInterface* storage_,
		const QueryProcessorInterface* processor_,
		MetaDataReaderInterface* metadata_,
		const std::string& type_,
		unsigned int sentencesize_,
		unsigned int windowsize_,
		unsigned int cardinality_,
		double nofCollectionDocuments_,
		const std::string& metadata_title_maxpos_,
		double maxdf_,
		const std::pair<std::string,std::string>& matchmark_,
		const std::pair<std::string,std::string>& floatingmark_,
		const std::string& name_para_,
		const std::string& name_phrase_,
		const std::string& name_docstart_,
		ErrorBufferInterface* errorhnd_)
	:m_storage(storage_)
	,m_processor(processor_)
	,m_metadata(metadata_)
	,m_forwardindex(storage_->createForwardIterator( type_))
	,m_type(type_)
	,m_sentencesize(sentencesize_)
	,m_windowsize(windowsize_)
	,m_cardinality(cardinality_)
	,m_nofCollectionDocuments(nofCollectionDocuments_)
	,m_metadata_title_maxpos(metadata_title_maxpos_.empty()?-1:metadata_->elementHandle( metadata_title_maxpos_))
	,m_maxdf(maxdf_)
	,m_matchmark(matchmark_)
	,m_floatingmark(floatingmark_)
	,m_name_para(name_para_)
	,m_name_phrase(name_phrase_)
	,m_name_docstart(name_docstart_)
	,m_itrarsize(0)
	,m_structarsize(0)
	,m_paraarsize(0)
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
			if (idf < 0.00001)
			{
				idf = 0.00001;
			}
			if (m_maxdf * m_nofCollectionDocuments < df)
			{
				m_maxdist_featar[ m_itrarsize] = (m_windowsize > 5)?5:m_windowsize;
			}
			else
			{
				m_maxdist_featar[ m_itrarsize] = m_windowsize;
			}
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

static Index callSkipPos( Index start, PostingIteratorInterface** ar, std::size_t size)
{
	Index rt = 0;
	std::size_t ti=0;
	for (; ti<size; ++ti)
	{
		Index pos = ar[ ti]->skipPos( start);
		if (pos)
		{
			if (!rt || pos < rt) rt = pos;
		}
	}
	return rt;
}

struct Candidate
{
	double weight;
	Index pos;
	Index span;

	Candidate( double weight_, Index pos_, Index span_)
		:weight(weight_),pos(pos_),span(span_){}
	Candidate( const Candidate& o)
		:weight(o.weight),pos(o.pos),span(o.span){}
};

static Candidate findCandidate(
			const Index& firstpos,
			const ProximityWeightAccumulator::WeightArray& idfar,
			const ProximityWeightAccumulator::WeightArray& weightincr,
			unsigned int maxwindowsize, unsigned int cardinality,
			PostingIteratorInterface** itrar, std::size_t itrarsize,
			PostingIteratorInterface** structar, std::size_t structarsize,
			PostingIteratorInterface** paraar, std::size_t parasize,
			const Index* maxdist_featar)
{
	Candidate rt( 0.0,firstpos,0);
	Index prevPara=firstpos, nextPara=callSkipPos( firstpos, paraar, parasize);
	PositionWindow poswin( itrar, itrarsize, maxwindowsize, cardinality,
				firstpos, PositionWindow::MaxWin);
	bool more = poswin.first();
	for (;more; more = poswin.next())
	{
		const std::size_t* window = poswin.window();
		std::size_t windowsize = poswin.size();
		Index windowpos = poswin.pos();
		Index windowspan = poswin.span();

		// Calculate the paragraph elements before and after the current window position:
		while (nextPara && nextPara < windowpos)
		{
			prevPara = nextPara;
			nextPara = callSkipPos( prevPara+1, paraar, parasize);
		}
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "weighting window " << windowpos << "..." << (windowpos+windowspan) << " para " << prevPara << "|" << nextPara << std::endl;
#endif
		// Check if window is overlapping a paragraph. In this case to not use it for summary:
		if (nextPara && nextPara < windowpos + windowspan) continue;

		// Calculate the weight of the current window:
		ProximityWeightAccumulator::WeightArray weightar( itrarsize, 1.0);

		ProximityWeightAccumulator::weight_same_sentence(
			weightar, 0.3, weightincr, window, windowsize,
			maxdist_featar, itrar, itrarsize, structar, structarsize);
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "\taccumulated ff incr [same sentence] " << weightar.tostring() << std::endl;
#endif
		ProximityWeightAccumulator::weight_imm_follow(
			weightar, 0.4, weightincr, window, windowsize, itrar, itrarsize);
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "\taccumulated ff incr [imm follow] " << weightar.tostring() << std::endl;
#endif
		ProximityWeightAccumulator::weight_invdist(
			weightar, 0.3, weightincr, window, windowsize, itrar, itrarsize);
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "\taccumulated ff incr [inv distance] " << weightar.tostring() << std::endl;
#endif
		if (windowpos - firstpos < 1000)
		{
			ProximityWeightAccumulator::weight_invpos(
				weightar, 0.5, weightincr, firstpos, itrar, itrarsize);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "\taccumulated ff incr [inv distance begin] " << weightar.tostring() << std::endl;
#endif
		}
		if (nextPara && windowpos >= nextPara)
		{
			// Weight inv distance to paragraph start:
			ProximityWeightAccumulator::weight_invpos(
				weightar, 0.3, weightincr, prevPara, itrar, itrarsize);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "\taccumulated ff incr [inv distance paragraph] " << weightar.tostring() << std::endl;
#endif
		}
		weightar.multiply( idfar);
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "\twindow feature weights: " << weightar.tostring() << std::endl;
#endif
		double weight = weightar.sum();
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "\twindow weight: " << weight << std::endl;
#endif
		// Select the best window:
		if (weight > rt.weight)
		{
			rt.weight = weight;
			rt.pos = windowpos;
			rt.span = windowspan;
		}
	}
	return rt;
}


std::vector<SummaryElement>
	SummarizerFunctionContextMatchPhrase::getSummary( const Index& docno)
{
	try
	{
		std::vector<SummaryElement> rt;
		if (!m_initialized)
		{
			if (m_itrarsize == 0)
			{
				return std::vector<SummaryElement>();
			}
			if (m_itrarsize < m_cardinality || m_cardinality == 0)
			{
				m_cardinality = m_itrarsize;
			}
			// initialize proportional ff increment weights
			m_weightincr.init( m_itrarsize);
			ProximityWeightAccumulator::proportionalAssignment( m_weightincr, 1.0, 0.3, m_idfar);
			m_initialized = true;
		}
		// Init document iterators:
		callSkipDoc( docno, m_itrar, m_itrarsize);
		callSkipDoc( docno, m_structar, m_structarsize);
		callSkipDoc( docno, m_paraar, m_paraarsize);
		m_forwardindex->skipDoc( docno);

		// Define search start position:
		Index firstpos = 1;
		if (m_metadata_title_maxpos>=0)
		{
			m_metadata->skipDoc( docno);
			NumericVariant firstposval = m_metadata->getValue( m_metadata_title_maxpos);
			firstpos = firstposval.toint()+1;
		}
		// Define best match to find:
		// Find the best match:
		Candidate candidate
			= findCandidate(
				firstpos, m_idfar, m_weightincr, m_windowsize, m_cardinality,
				m_itrar, m_itrarsize, m_structar, m_structarsize, m_paraar, m_paraarsize, 
				m_maxdist_featar);
		if (candidate.span == 0 && m_metadata_title_maxpos>=0)
		{
			//... we did not find a summary with m_cardinality terms, so we try to find one
			//	without the terms appearing in the document title:
			PostingIteratorInterface* noTitleTerms[ MaxNofArguments];
			ProximityWeightAccumulator::WeightArray noTitleIdfs;
			ProximityWeightAccumulator::WeightArray noWeightIncrs;
			std::size_t noTitleSize = 0;
			std::size_t ti=0,te=m_itrarsize;
			for (; ti < te; ++ti)
			{
				Index pos = m_itrar[ti]->skipPos( 0);
				if (pos >= firstpos)
				{
					noTitleTerms[ noTitleSize++] = m_itrar[ ti];
					noTitleIdfs.add( m_idfar[ ti]);
					noWeightIncrs.add( m_weightincr[ ti]);
				}
			}
			if (noTitleSize && m_itrarsize > noTitleSize)
			{
				unsigned int cardinality = noTitleSize;
				if (m_cardinality < cardinality)
				{
					cardinality = m_cardinality;
				}
				candidate = findCandidate(
						firstpos, noTitleIdfs, noWeightIncrs, m_windowsize, cardinality,
						noTitleTerms, noTitleSize, m_structar, m_structarsize,
						m_paraar, m_paraarsize, m_maxdist_featar);
			}
		}
		bool is_docstart = (candidate.span == 0);
		if (is_docstart)
		{
			// ... if we did not find any candidate window, we take the start of the document as abstract:
			candidate.pos = firstpos;
			candidate.span = 10;
		}
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "candidate found pos=" << candidate.pos << ", span=" << candidate.span << " is docstart " << (is_docstart?"YES":"NO") << std::endl;
#endif
		struct Abstract
		{
			Index start;
			Index span;
			bool defined_start;
			bool defined_end;
			bool is_docstart;
		};
		Abstract phrase_abstract = {candidate.pos,candidate.span,(candidate.pos == firstpos),false,is_docstart};

		// Find the start of the abstract as start of the preceeding structure or paragraph marker:
		std::size_t ti=0,te=m_paraarsize+m_structarsize;
		Index astart;
		if ((Index)m_sentencesize + firstpos < candidate.pos)
		{
			astart = candidate.pos - (Index)m_sentencesize;
		}
		else
		{
			astart = firstpos;
			phrase_abstract.defined_start = true;
		}
		for (; ti<te && astart < candidate.pos; ++ti)
		{
			Index pos = m_structar[ti]->skipPos( astart);
			while (pos > astart && pos <= candidate.pos)
			{
				phrase_abstract.defined_start = true;
				astart = pos + 1;
				pos = m_structar[ti]->skipPos( astart);
			}
		}
		phrase_abstract.span += candidate.pos - astart;
		phrase_abstract.start = astart;
		// Find the end of the abstract, by scanning for the next sentence:
		if (phrase_abstract.span < (Index)m_sentencesize)
		{
			Index minincr = 0;
			if (phrase_abstract.span < ((Index)m_sentencesize >> 2))
			{
				minincr += (Index)m_sentencesize >> 2;
				// .... heuristics for minimal size of abstract we want to show
			}
			Index nextparapos = callSkipPos( phrase_abstract.start + phrase_abstract.span, m_paraar, m_paraarsize);
			Index eospos = callSkipPos( phrase_abstract.start + phrase_abstract.span + minincr, m_structar, m_structarsize + m_paraarsize);
			if (eospos)
			{
				if (nextparapos && nextparapos <= eospos) eospos = nextparapos - 1;
				if ((eospos - phrase_abstract.start) < (Index)m_sentencesize)
				{
					phrase_abstract.span = eospos - phrase_abstract.start + 1;
					phrase_abstract.defined_end = true;
				}
				else
				{
					phrase_abstract.span = m_sentencesize;
				}
			}
		}
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "phrase abstract pos=" << phrase_abstract.start << ", span=" << phrase_abstract.span << " defined start " << (phrase_abstract.defined_start?"YES":"NO") << " defined end " << (phrase_abstract.defined_end?"YES":"NO") << std::endl;
#endif
		// Find the title of the paragraph the phrase belongs to:
		Index searchrange = 200;
		Index parapos = 0;
		Index eoppos = 0;
		Abstract para_abstract = {0,0,true,true,false};
		for (;;)
		{
			Index pstart = phrase_abstract.start  > searchrange + firstpos ? phrase_abstract.start - searchrange : (firstpos + 1);
			Index prevparapos = callSkipPos( pstart, m_paraar, m_paraarsize);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "start search candidate para pos=" << pstart << ", nextpara=" << prevparapos << std::endl;
#endif
			if (prevparapos && prevparapos <= phrase_abstract.start)
			{
				parapos = prevparapos;
#ifdef STRUS_LOWLEVEL_DEBUG
				std::cout << "found candidate para pos=" << parapos << std::endl;
#endif
				for (;;)
				{
					prevparapos = callSkipPos( parapos+1, m_paraar, m_paraarsize);
					if (prevparapos && prevparapos <= phrase_abstract.start)
					{
						parapos = prevparapos;
					}
					else
					{
						break;
					}
				}
				eoppos = callSkipPos( parapos+1, m_structar, m_structarsize + m_paraarsize);
				if (eoppos && eoppos - parapos < 12)
				{
					if (eoppos >= phrase_abstract.start)
					{
						if (eoppos >= phrase_abstract.start)
						{
							if (eoppos < phrase_abstract.start + phrase_abstract.span)
							{
								phrase_abstract.span -= (eoppos - phrase_abstract.start);
								phrase_abstract.start = eoppos+1;
							}
							else
							{
								phrase_abstract.span = 0;
							}
						}
						para_abstract.start = parapos;
						para_abstract.span = (eoppos-parapos+1);
					}
					else if (parapos != firstpos)
					{
						para_abstract.start = parapos;
						para_abstract.span = (eoppos-parapos+1);
					}
				}
				break;
			}
			else
			{
				if (pstart <= (firstpos + 1)) break;
				searchrange *= 2;
			}
		}
		
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "para abstract pos=" << para_abstract.start << ", span=" << para_abstract.span << std::endl;
#endif
		// Create the paragraph result, if exists:
		if (para_abstract.span > 0)
		{
			std::string paratitle;
			Index pi = para_abstract.start, pe = para_abstract.start + para_abstract.span;
			for (; pi < pe; ++pi)
			{
				if (m_forwardindex->skipPos(pi) == pi)
				{
					if (!paratitle.empty()) paratitle.push_back(' ');
					paratitle.append( m_forwardindex->fetch());
				}
			}
			rt.push_back( SummaryElement( m_name_para, paratitle, 1.0));
		}
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "firstpos=" << firstpos << std::endl;
#endif
		// Create the highlighted phrase result, if exists:
		if (phrase_abstract.span > 0)
		{
			// Create the array of positions to highlight:
			std::vector<Index> highlightpos;
			Index pi = phrase_abstract.start, pe = phrase_abstract.start + phrase_abstract.span;
			while (pi < pe)
			{
				Index minpos = callSkipPos( pi, m_itrar, m_itrarsize);
				if (minpos && minpos < pe)
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
			if (!phrase_abstract.defined_start)
			{
				phrase.append( m_floatingmark.first);
			}
			pi = phrase_abstract.start, pe = phrase_abstract.start + phrase_abstract.span;
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
					for (; hi < he && pi > highlightpos[hi]; ++hi){}
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
			if (!phrase_abstract.defined_end)
			{
				phrase.append( m_floatingmark.second);
			}
			if (phrase_abstract.is_docstart)
			{
				rt.push_back( SummaryElement( m_name_docstart, phrase, 1.0));
			}
			else
			{
				rt.push_back( SummaryElement( m_name_phrase, phrase, 1.0));
			}
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summary: %s"), "matchphrase", *m_errorhnd, std::vector<SummaryElement>());
}

static std::pair<std::string,std::string> parseMarker( const std::string& value)
{
	std::pair<std::string,std::string> rt;
	if (!value.empty())
	{
		char sep = value[0];
		char const* mid = std::strchr( value.c_str()+1, sep);
		if (mid)
		{
			rt.first = std::string( value.c_str()+1, mid - value.c_str() - 1);
			rt.second = std::string( mid+1);
		}
		else
		{
			rt.first = value;
			rt.second = value;
		}
	}
	return rt;
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
		else if (utils::caseInsensitiveEquals( name, "metadata_title_maxpos"))
		{
			m_metadata_title_maxpos = value;
		}
		else if (utils::caseInsensitiveEquals( name, "matchmark"))
		{
			m_matchmark = parseMarker( value);
		}
		else if (utils::caseInsensitiveEquals( name, "floatingmark"))
		{
			m_floatingmark = parseMarker( value);
		}
		else if (utils::caseInsensitiveEquals( name, "name_para"))
		{
			m_name_para = value;
		}
		else if (utils::caseInsensitiveEquals( name, "name_phrase"))
		{
			m_name_phrase = value;
		}
		else if (utils::caseInsensitiveEquals( name, "name_docstart"))
		{
			m_name_docstart = value;
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

void SummarizerFunctionInstanceMatchPhrase::addNumericParameter( const std::string& name, const NumericVariant& value)
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
	else if (utils::caseInsensitiveEquals( name, "maxdf"))
	{
		m_maxdf = (double)value;
		if (m_maxdf < 0.0 || m_maxdf > 1.0)
		{
			m_errorhnd->report( _TXT("parameter '%s' for weighting scheme '%s' expected to a positive floating point number between 0.0 and 1.0"), name.c_str(), "matchphrase");
		}
	}
	else if (utils::caseInsensitiveEquals( name, "type")
		|| utils::caseInsensitiveEquals( name, "matchmark")
		|| utils::caseInsensitiveEquals( name, "floatingmark")
		|| utils::caseInsensitiveEquals( name, "name_para")
		|| utils::caseInsensitiveEquals( name, "name_phrase")
		|| utils::caseInsensitiveEquals( name, "name_docstart")
		|| utils::caseInsensitiveEquals( name, "metadata_title_maxpos"))
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
				storage, m_processor, metadata, m_type,
				m_sentencesize, m_windowsize, m_cardinality,
				nofCollectionDocuments, m_metadata_title_maxpos, m_maxdf,
				m_matchmark, m_floatingmark,
				m_name_para, m_name_phrase, m_name_docstart,
				m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' summarizer: %s"), "matchphrase", *m_errorhnd, 0);
}

std::string SummarizerFunctionInstanceMatchPhrase::tostring() const
{
	try
	{
		std::ostringstream rt;
		rt << "type='" << m_type << "'"
			<< ", matchmark=(" << m_matchmark.first << "," << m_matchmark.second << ")"
			<< ", floatingmark=(" << m_floatingmark.first << "," << m_floatingmark.second << ")"
			<< ", name_para=" << m_name_para << ", name_phrase=" << m_name_phrase
			<< ", metadata_title_maxpos='" << m_metadata_title_maxpos << "'"
			<< ", sentencesize='" << m_sentencesize << "'"
			<< ", windowsize='" << m_windowsize << "'"
			<< ", cardinality='" << m_cardinality << "'";
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

FunctionDescription SummarizerFunctionMatchPhrase::getDescription() const
{
	try
	{
		typedef FunctionDescription::Parameter P;
		FunctionDescription rt( _TXT("Get best matching phrases delimited by the structure postings"));
		rt( P::Feature, "match", _TXT( "defines the features to weight"), "");
		rt( P::Feature, "struct", _TXT( "defines the delimiter for structures"), "");
		rt( P::Feature, "para", _TXT( "defines the delimiter for paragraphs (summaries must not overlap paragraph borders)"), "");
		rt( P::String, "type", _TXT( "the forward index type of the result phrase elements"), "");
		rt( P::Metadata, "metadata_title_maxpos", _TXT( "the metadata element that specifies the last title element. Only content is used for abstracting"), "1:");
		rt( P::Numeric, "sentencesize", _TXT( "restrict the maximum length of sentences in summaries"), "1:");
		rt( P::Numeric, "windowsize", _TXT( "maximum size of window used for identifying matches"), "1:");
		rt( P::Numeric, "cardinality", _TXT( "minimum number of features in a window"), "1:");
		rt( P::String, "matchmark", _TXT( "specifies the markers (first character of the value is the separator followed by the two parts separated by it) for highlighting matches in the resulting phrases"), "");
		rt( P::String, "floatingmark", _TXT( "specifies the markers (first character of the value is the separator followed by the two parts separated by it) for marking floating phrases without start or end of sentence found"), "");
		rt( P::String, "name_para", _TXT( "specifies the summary element name used for paragraphs (default 'para')"), "");
		rt( P::String, "name_phrase", _TXT( "specifies the summary element name used for phrases (default 'phrase')"), "");
		rt( P::String, "name_docstart", _TXT( "specifies the summary element name used for the document start (alternative summary, if no match found, default 'docstart')"), "");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating summarizer function description for '%s': %s"), "matchphrase", *m_errorhnd, FunctionDescription());
}

