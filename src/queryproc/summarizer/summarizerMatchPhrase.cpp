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

#define METHOD_NAME "matchphrase"

SummarizerFunctionContextMatchPhrase::SummarizerFunctionContextMatchPhrase(
		const StorageClientInterface* storage_,
		const QueryProcessorInterface* processor_,
		MetaDataReaderInterface* metadata_,
		const Reference<SummarizerFunctionParameterMatchPhrase>& parameter_,
		double nofCollectionDocuments_,
		ErrorBufferInterface* errorhnd_)
	:m_storage(storage_)
	,m_processor(processor_)
	,m_metadata(metadata_)
	,m_forwardindex(storage_->createForwardIterator(parameter_->m_type))
	,m_parameter(parameter_)
	,m_nofCollectionDocuments(nofCollectionDocuments_)
	,m_itrarsize(0)
	,m_structarsize(0)
	,m_paraarsize(0)
	,m_cardinality(parameter_->m_cardinality)
	,m_initialized(false)
	,m_titleitr(0)
	,m_errorhnd(errorhnd_)
{
	if (!m_forwardindex.get()) throw strus::runtime_error(_TXT("error creating forward index iterator"));
}


void SummarizerFunctionContextMatchPhrase::addSummarizationFeature(
		const std::string& name,
		PostingIteratorInterface* itr,
		const std::vector<SummarizationVariable>&,
		double weight,
		const TermStatistics& termstats)
{
	try
	{
		if (utils::caseInsensitiveEquals( name, "title"))
		{
			if (m_titleitr) throw strus::runtime_error(_TXT("title field specified twice"));
			m_titleitr = itr;
		}
		else if (utils::caseInsensitiveEquals( name, "struct"))
		{
			if (m_paraarsize + m_structarsize > MaxNofArguments) throw strus::runtime_error( _TXT("number of structure features out of range"));
			m_structar[ m_structarsize + m_paraarsize] = m_structar[ m_structarsize];
			m_structar[ m_structarsize++] = itr;
		}
		else if (utils::caseInsensitiveEquals( name, "para"))
		{
			if (m_paraarsize + m_structarsize > MaxNofArguments) throw strus::runtime_error( _TXT("number of structure features out of range"));
			m_structar[ m_structarsize + m_paraarsize] = itr;
			m_paraarsize++;
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
			if (m_parameter->m_maxdf * m_nofCollectionDocuments < df)
			{
				m_maxdist_featar[ m_itrarsize] = (m_parameter->m_windowsize > 5)?5:m_parameter->m_windowsize;
			}
			else
			{
				m_maxdist_featar[ m_itrarsize] = m_parameter->m_windowsize;
			}
			m_itrar[ m_itrarsize++] = itr;
			m_idfar.add( idf * weight);
		}
		else
		{
			m_errorhnd->report( _TXT("unknown '%s' summarization function parameter '%s'"), METHOD_NAME, name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding feature to '%s' summarizer: %s"), METHOD_NAME, *m_errorhnd);
}

SummarizerFunctionContextMatchPhrase::~SummarizerFunctionContextMatchPhrase()
{}


static void callSkipDoc( const Index& docno, PostingIteratorInterface** ar, std::size_t arsize, PostingIteratorInterface** valid_ar)
{
	for (std::size_t ai=0; ai < arsize; ++ai)
	{
		if (docno == ar[ ai]->skipDoc( docno))
		{
			valid_ar[ ai] = ar[ ai];
		}
		else
		{
			valid_ar[ ai] = 0;
		}
	}
}

static Index callSkipPos( Index start, PostingIteratorInterface** ar, std::size_t size)
{
	Index rt = 0;
	std::size_t ti=0;
	for (; ti<size; ++ti)
	{
		if (ar[ ti])
		{
			Index pos = ar[ ti]->skipPos( start);
			if (pos)
			{
				if (!rt || pos < rt) rt = pos;
			}
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

		if (itrarsize == 1)
		{
			weightar[ 0] += 1.0;
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "\tff incr [single feature] " << result.tostring() << std::endl;
#endif
		}
		else
		{
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
		}
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
			// initialize proportional ff increment weights
			m_weightincr.init( m_itrarsize);
			ProximityWeightAccumulator::proportionalAssignment( m_weightincr, 1.0, 0.3, m_idfar);

			if (m_cardinality == 0)
			{
				if (m_parameter->m_cardinality_frac > std::numeric_limits<double>::epsilon())
				{
					m_cardinality = std::min( 1U, (unsigned int)(m_itrarsize * m_parameter->m_cardinality_frac + 0.5));
				}
				else
				{
					m_cardinality = m_itrarsize;
				}
			}
			m_initialized = true;
		}
		if (m_itrarsize < m_cardinality)
		{
			return rt;
		}
		// Init document iterators:
		PostingIteratorInterface* valid_itrar[ MaxNofArguments];			//< valid array if weighted features
		PostingIteratorInterface* valid_structar[ MaxNofArguments];			//< valid array of end of structure elements
		PostingIteratorInterface** valid_paraar = &valid_structar[ m_structarsize];	//< valid array of end of paragraph elements

		callSkipDoc( docno, m_itrar, m_itrarsize, valid_itrar);
		callSkipDoc( docno, m_structar, m_structarsize + m_paraarsize, valid_structar);
		m_forwardindex->skipDoc( docno);

		// Define search start position:
		Index titleStart = 1;
		Index titleEnd = 1;
		if (m_titleitr && m_titleitr->skipDoc( docno) == docno)
		{
			titleStart = m_titleitr->skipPos(0);
			if (titleStart)
			{
				Index ti = titleEnd = titleStart;
				while (0!=(ti=m_titleitr->skipPos(ti+1)))
				{
					titleEnd = ti;
				}
				++titleEnd;
			}
			else
			{
				titleStart = 1;
			}
		}
		Index firstpos = m_forwardindex->skipPos( titleEnd);
		if (!firstpos) firstpos = titleEnd;

		// Define best match to find:
		// Find the best match:
		Candidate candidate
			= findCandidate(
				firstpos, m_idfar, m_weightincr, m_parameter->m_windowsize, m_cardinality,
				valid_itrar, m_itrarsize, valid_structar, m_structarsize, valid_paraar, m_paraarsize, 
				m_maxdist_featar);
		if (candidate.span == 0 && m_titleitr)
		{
			//... we did not find a summary with m_cardinality terms, so we try to find one
			//	without the terms appearing in the document title:
			PostingIteratorInterface* noTitleTerms[ MaxNofArguments];
			ProximityWeightAccumulator::WeightArray noTitleIdfs;
			ProximityWeightAccumulator::WeightArray noWeightIncrs;
			std::size_t noTitleSize = 0;
			std::size_t cntTitleTerms = 0;
			std::size_t ti=0,te=m_itrarsize;
			for (; ti < te; ++ti)
			{
				if (valid_itrar[ti])
				{
					Index pos = valid_itrar[ti]->skipPos( 0);
					if (pos >= firstpos)
					{
						noTitleTerms[ noTitleSize++] = m_itrar[ ti];
						noTitleIdfs.add( m_idfar[ ti]);
						noWeightIncrs.add( m_weightincr[ ti]);
					}
					else
					{
						++cntTitleTerms;
					}
				}
			}
			if (noTitleSize && m_itrarsize > noTitleSize)
			{
				unsigned int cardinality = m_cardinality > cntTitleTerms ? (m_cardinality - cntTitleTerms) : 1;
				candidate = findCandidate(
						firstpos, noTitleIdfs, noWeightIncrs, m_parameter->m_windowsize, cardinality,
						noTitleTerms, noTitleSize, valid_structar, m_structarsize,
						valid_paraar, m_paraarsize, m_maxdist_featar);
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
		if ((Index)m_parameter->m_sentencesize + firstpos < candidate.pos)
		{
			astart = candidate.pos - (Index)m_parameter->m_sentencesize;
		}
		else
		{
			astart = firstpos;
			phrase_abstract.defined_start = true;
		}
		for (; ti<te && astart < candidate.pos; ++ti)
		{
			if (valid_structar[ti])
			{
				Index pos = valid_structar[ti]->skipPos( astart);
				while (pos > astart && pos <= candidate.pos)
				{
					phrase_abstract.defined_start = true;
					astart = pos + 1;
					pos = valid_structar[ti]->skipPos( astart);
				}
			}
		}
		phrase_abstract.span += candidate.pos - astart;
		phrase_abstract.start = astart;
		// Find the end of the abstract, by scanning for the next sentence:
		if (phrase_abstract.span < (Index)m_parameter->m_sentencesize)
		{
			Index minincr = 0;
			if (phrase_abstract.span < ((Index)m_parameter->m_sentencesize >> 2))
			{
				minincr += (Index)m_parameter->m_sentencesize >> 2;
				// .... heuristics for minimal size of abstract we want to show
			}
			Index nextparapos = callSkipPos( phrase_abstract.start + phrase_abstract.span, valid_paraar, m_paraarsize);
			Index eospos = callSkipPos( phrase_abstract.start + phrase_abstract.span + minincr, valid_structar, m_structarsize + m_paraarsize);
			if (eospos)
			{
				if (nextparapos && nextparapos <= eospos) eospos = nextparapos - 1;
				if ((eospos - phrase_abstract.start) < (Index)m_parameter->m_sentencesize)
				{
					phrase_abstract.span = eospos - phrase_abstract.start + 1;
					phrase_abstract.defined_end = true;
				}
				else
				{
					phrase_abstract.span = m_parameter->m_sentencesize;
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
			Index prevparapos = callSkipPos( pstart, valid_paraar, m_paraarsize);
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
					prevparapos = callSkipPos( parapos+1, valid_paraar, m_paraarsize);
					if (prevparapos && prevparapos <= phrase_abstract.start)
					{
						parapos = prevparapos;
					}
					else
					{
						break;
					}
				}
				eoppos = callSkipPos( parapos+1, valid_structar, m_structarsize + m_paraarsize);
				if (eoppos && eoppos - parapos < 12)
				{
					if (eoppos >= phrase_abstract.start)
					{
						if (eoppos >= phrase_abstract.start)
						{
							if (eoppos < phrase_abstract.start + phrase_abstract.span)
							{
								phrase_abstract.span = phrase_abstract.span - (eoppos - phrase_abstract.start);
								phrase_abstract.start = eoppos;
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
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "build paragraph title '" << paratitle << "'" << std::endl;
#endif
			rt.push_back( SummaryElement( m_parameter->m_name_para, paratitle, 1.0));
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
				Index minpos = callSkipPos( pi, valid_itrar, m_itrarsize);
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
				phrase.append( m_parameter->m_floatingmark.first);
			}
			pi = phrase_abstract.start, pe = phrase_abstract.start + phrase_abstract.span;
			if (pi > pe || (unsigned int)(pe - pi) > 2*m_parameter->m_sentencesize+10)
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
						phrase.append( m_parameter->m_matchmark.first);
						phrase.append( m_forwardindex->fetch());
						phrase.append( m_parameter->m_matchmark.second);
					}
					else
					{
						phrase.append( m_forwardindex->fetch());
					}
				}
			}
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "build phrase " << phrase_abstract.start << ".." << (phrase_abstract.start + phrase_abstract.span) << ": '" << phrase << "'" << std::endl;
#endif
			if (!phrase_abstract.defined_end)
			{
				phrase.append( m_parameter->m_floatingmark.second);
			}
			if (phrase_abstract.is_docstart)
			{
				rt.push_back( SummaryElement( m_parameter->m_name_docstart, phrase, 1.0));
			}
			else
			{
				rt.push_back( SummaryElement( m_parameter->m_name_phrase, phrase, 1.0));
			}
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summary: %s"), METHOD_NAME, *m_errorhnd, std::vector<SummaryElement>());
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
		if (utils::caseInsensitiveEquals( name, "match") || utils::caseInsensitiveEquals( name, "struct") || utils::caseInsensitiveEquals( name, "para") || utils::caseInsensitiveEquals( name, "title"))
		{
			m_errorhnd->report( _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as string"), name.c_str(), "matchvariables");
		}
		else if (utils::caseInsensitiveEquals( name, "type"))
		{
			m_parameter->m_type = value;
		}
		else if (utils::caseInsensitiveEquals( name, "matchmark"))
		{
			m_parameter->m_matchmark = parseMarker( value);
		}
		else if (utils::caseInsensitiveEquals( name, "floatingmark"))
		{
			m_parameter->m_floatingmark = parseMarker( value);
		}
		else if (utils::caseInsensitiveEquals( name, "sentencesize"))
		{
			m_errorhnd->report( _TXT("no string value expected for parameter '%s' in summarization function '%s'"), name.c_str(), METHOD_NAME);
		}
		else if (utils::caseInsensitiveEquals( name, "windowsize"))
		{
			m_errorhnd->report( _TXT("no string value expected for parameter '%s' in summarization function '%s'"), name.c_str(), METHOD_NAME);
		}
		else if (utils::caseInsensitiveEquals( name, "cardinality") && !value.empty() && value[value.size()-1] == '%')
		{
			m_parameter->m_cardinality = 0;
			m_parameter->m_cardinality_frac = utils::tofraction( value);
		}
		else
		{
			m_errorhnd->report( _TXT("unknown '%s' summarization function parameter '%s'"), METHOD_NAME, name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding string parameter to '%s' summarizer: %s"), METHOD_NAME, *m_errorhnd);
}

void SummarizerFunctionInstanceMatchPhrase::addNumericParameter( const std::string& name, const NumericVariant& value)
{
	if (utils::caseInsensitiveEquals( name, "match") || utils::caseInsensitiveEquals( name, "struct") || utils::caseInsensitiveEquals( name, "para") || utils::caseInsensitiveEquals( name, "title"))
	{
		m_errorhnd->report( _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as numeric value"), name.c_str(), METHOD_NAME);
	}
	else if (utils::caseInsensitiveEquals( name, "sentencesize"))
	{
		m_parameter->m_sentencesize = (unsigned int)value;
	}
	else if (utils::caseInsensitiveEquals( name, "windowsize"))
	{
		m_parameter->m_windowsize = (unsigned int)value;
	}
	else if (utils::caseInsensitiveEquals( name, "cardinality"))
	{
		m_parameter->m_cardinality = (unsigned int)value;
		m_parameter->m_cardinality_frac = 0.0;
	}
	else if (utils::caseInsensitiveEquals( name, "maxdf"))
	{
		m_parameter->m_maxdf = (double)value;
		if (m_parameter->m_maxdf < 0.0 || m_parameter->m_maxdf > 1.0)
		{
			m_errorhnd->report( _TXT("parameter '%s' for weighting scheme '%s' expected to a positive floating point number between 0.0 and 1.0"), name.c_str(), METHOD_NAME);
		}
	}
	else if (utils::caseInsensitiveEquals( name, "type")
		|| utils::caseInsensitiveEquals( name, "matchmark")
		|| utils::caseInsensitiveEquals( name, "floatingmark")
		|| utils::caseInsensitiveEquals( name, "metadata_title_maxpos"))
	{
		m_errorhnd->report( _TXT("no numeric value expected for parameter '%s' in summarization function '%s'"), name.c_str(), METHOD_NAME);
	}
	else
	{
		m_errorhnd->report( _TXT("unknown '%s' summarization function parameter '%s'"), METHOD_NAME, name.c_str());
	}
}

void SummarizerFunctionInstanceMatchPhrase::defineResultName(
		const std::string& resultname,
		const std::string& itemname)
{
	try
	{
		if (utils::caseInsensitiveEquals( itemname, "para"))
		{
			m_parameter->m_name_para = resultname;
		}
		else if (utils::caseInsensitiveEquals( itemname, "phrase"))
		{
			m_parameter->m_name_phrase = resultname;
		}
		else if (utils::caseInsensitiveEquals( itemname, "docstart"))
		{
			m_parameter->m_name_docstart = resultname;
		}
		else
		{
			throw strus::runtime_error( _TXT("unknown item name '%s"), itemname.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error defining result name of '%s' summarizer: %s"), METHOD_NAME, *m_errorhnd);
}


SummarizerFunctionContextInterface* SummarizerFunctionInstanceMatchPhrase::createFunctionContext(
		const StorageClientInterface* storage,
		MetaDataReaderInterface* metadata,
		const GlobalStatistics& stats) const
{
	if (m_parameter->m_type.empty())
	{
		m_errorhnd->report( _TXT( "emtpy term type definition (parameter 'type') in match phrase summarizer configuration"));
	}
	try
	{
		double nofCollectionDocuments = stats.nofDocumentsInserted()>=0?stats.nofDocumentsInserted():(GlobalCounter)storage->nofDocumentsInserted();
		return new SummarizerFunctionContextMatchPhrase(
				storage, m_processor, metadata, m_parameter,
				nofCollectionDocuments, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' summarizer: %s"), METHOD_NAME, *m_errorhnd, 0);
}

std::string SummarizerFunctionInstanceMatchPhrase::tostring() const
{
	try
	{
		std::ostringstream rt;
		rt << "type='" << m_parameter->m_type << "'"
			<< ", matchmark=(" << m_parameter->m_matchmark.first << "," << m_parameter->m_matchmark.second << ")"
			<< ", floatingmark=(" << m_parameter->m_floatingmark.first << "," << m_parameter->m_floatingmark.second << ")"
			<< ", name_para=" << m_parameter->m_name_para << ", name_phrase=" << m_parameter->m_name_phrase
			<< ", sentencesize='" << m_parameter->m_sentencesize << "'"
			<< ", windowsize='" << m_parameter->m_windowsize << "'"
			<< ", cardinality='" << m_parameter->m_cardinality << "'"
			<< ", maxdf='" << m_parameter->m_maxdf << "'";
		return rt.str();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error mapping '%s' summarizer to string: %s"), METHOD_NAME, *m_errorhnd, std::string());
}


SummarizerFunctionInstanceInterface* SummarizerFunctionMatchPhrase::createInstance(
		const QueryProcessorInterface* processor) const
{
	try
	{
		return new SummarizerFunctionInstanceMatchPhrase( processor, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of '%s' summarizer: %s"), METHOD_NAME, *m_errorhnd, 0);
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
		rt( P::Numeric, "maxdf", _TXT( "the maximum df (fraction of collection size) of features considered for same sentence proximity weighing"), "1:");
		rt( P::String, "matchmark", _TXT( "specifies the markers (first character of the value is the separator followed by the two parts separated by it) for highlighting matches in the resulting phrases"), "");
		rt( P::String, "floatingmark", _TXT( "specifies the markers (first character of the value is the separator followed by the two parts separated by it) for marking floating phrases without start or end of sentence found"), "");
		rt( P::String, "name_para", _TXT( "specifies the summary element name used for paragraphs (default 'para')"), "");
		rt( P::String, "name_phrase", _TXT( "specifies the summary element name used for phrases (default 'phrase')"), "");
		rt( P::String, "name_docstart", _TXT( "specifies the summary element name used for the document start (alternative summary, if no match found, default 'docstart')"), "");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating summarizer function description for '%s': %s"), METHOD_NAME, *m_errorhnd, FunctionDescription());
}

