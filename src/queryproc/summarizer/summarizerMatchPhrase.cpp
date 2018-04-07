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
#include "postingIteratorHelpers.hpp"
#include "strus/numericVariant.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/constants.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/string_conv.hpp"
#include "strus/base/numstring.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <cmath>

using namespace strus;

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
	,m_nof_maxdf_features(0)
	,m_cardinality(parameter_->m_cardinality)
	,m_initialized(false)
	,m_titleitr(0)
	,m_errorhnd(errorhnd_)
{
	if (!m_forwardindex.get()) throw std::runtime_error( _TXT("error creating forward index iterator"));
}

void SummarizerFunctionContextMatchPhrase::setVariableValue( const std::string&, double)
{
	m_errorhnd->report( ErrorCodeNotImplemented, _TXT("no variables known for function '%s'"), METHOD_NAME);
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
		if (strus::caseInsensitiveEquals( name, "title"))
		{
			if (m_titleitr) throw std::runtime_error( _TXT("title field specified twice"));
			m_titleitr = itr;
		}
		else if (strus::caseInsensitiveEquals( name, "struct"))
		{
			if (m_paraarsize + m_structarsize > MaxNofArguments) throw strus::runtime_error( "%s",  _TXT("number of structure features out of range"));
			m_structar[ m_structarsize + m_paraarsize] = m_structar[ m_structarsize];
			m_structar[ m_structarsize++] = itr;
		}
		else if (strus::caseInsensitiveEquals( name, "para"))
		{
			if (m_paraarsize + m_structarsize > MaxNofArguments) throw strus::runtime_error( "%s",  _TXT("number of structure features out of range"));
			m_structar[ m_structarsize + m_paraarsize] = itr;
			m_paraarsize++;
		}
		else if (strus::caseInsensitiveEquals( name, "match"))
		{
			if (m_itrarsize > MaxNofArguments) throw strus::runtime_error( "%s",  _TXT("number of weighting features out of range"));

			double df = termstats.documentFrequency()>=0?termstats.documentFrequency():(GlobalCounter)itr->documentFrequency();
			double idf = logl( (m_nofCollectionDocuments - df + 0.5) / (df + 0.5));
			if (idf < 0.00001)
			{
				idf = 0.00001;
			}
			if (m_parameter->m_maxdf * m_nofCollectionDocuments < df)
			{
				++m_nof_maxdf_features;
			}
			m_itrar[ m_itrarsize++] = itr;
			m_idfar.push( idf * weight);
		}
		else
		{
			m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' summarization function parameter '%s'"), METHOD_NAME, name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding feature to '%s' summarizer: %s"), METHOD_NAME, *m_errorhnd);
}

SummarizerFunctionContextMatchPhrase::~SummarizerFunctionContextMatchPhrase()
{}

double SummarizerFunctionContextMatchPhrase::windowWeight(
		WeightingData& wdata, const PositionWindow& poswin,
		const std::pair<Index,Index>& structframe,
		const std::pair<Index,Index>& paraframe)
{
	const std::size_t* window = poswin.window();
	std::size_t windowsize = poswin.size();
	Index windowpos = poswin.pos();

	// Calculate the weight of the current window:
	ProximityWeightAccumulator::WeightArray weightar( m_itrarsize, 1.0);

	if (m_itrarsize == 1)
	{
		weightar[ 0] += 1.0;
	}
	else
	{
		ProximityWeightAccumulator::weight_same_sentence(
			weightar, m_parameter->m_weight_same_sentence, m_weightincr,
			window, windowsize,
			wdata.valid_itrar, m_itrarsize,
			structframe);
		ProximityWeightAccumulator::weight_invdist(
			weightar, m_parameter->m_weight_invdist, m_weightincr,
			window, windowsize,
			wdata.valid_itrar, m_itrarsize);
	}
	if (windowpos < 1000)
	{
		// Weight distance to start of document:
		ProximityWeightAccumulator::weight_invpos(
			weightar, m_parameter->m_weight_invpos_start, m_weightincr, 1,
			window, windowsize, wdata.valid_itrar, m_itrarsize);
	}
	if (paraframe.first)
	{
		// Weight inv distance to paragraph start:
		ProximityWeightAccumulator::weight_invpos(
			weightar, m_parameter->m_weight_invpos_para, m_weightincr, paraframe.first,
			window, windowsize, wdata.valid_itrar, m_itrarsize);
	}
	if (structframe.first)
	{
		// Weight inv distance to paragraph start:
		ProximityWeightAccumulator::weight_invpos(
			weightar, m_parameter->m_weight_invpos_struct, m_weightincr, structframe.first,
			window, windowsize, wdata.valid_itrar, m_itrarsize);
	}
	weightar.multiply( m_idfar);
	return weightar.sum();
}

SummarizerFunctionContextMatchPhrase::Match SummarizerFunctionContextMatchPhrase::findBestMatch_(
			WeightingData& wdata,
			unsigned int cardinality,
			PostingIteratorInterface** itrar)
{
	Match rt;
	Index lastEndPos = 0;
	unsigned int lastElementCnt = 0;
	Index firstpos = wdata.titleend;
	PositionWindow poswin( itrar, m_itrarsize, m_parameter->m_windowsize, cardinality,
				firstpos, PositionWindow::MaxWin);
	bool more = poswin.first();
	for (;more; more = poswin.next())
	{
		// Check if the window is a really new one and not one already covered:
		Index windowpos = poswin.pos();
		Index windowspan = poswin.span();
		if (lastEndPos == windowpos + windowspan && lastElementCnt > poswin.size())
		{
			continue;
		}
		lastElementCnt = poswin.size();
		lastEndPos = windowpos + windowspan;

		// Check if window is overlapping a paragraph. In this case to not use it for summary:
		std::pair<Index,Index> paraframe = wdata.paraiter.skipPos( windowpos);
		if (paraframe.first && paraframe.second < windowpos + windowspan) continue;

		// Calculate sentence frame:
		std::pair<Index,Index> structframe = wdata.structiter.skipPos( windowpos);

		// Calculate the candidate weight:
		double weight = windowWeight( wdata, poswin, structframe, paraframe);

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

std::string SummarizerFunctionContextMatchPhrase::getPhraseString( const Index& firstpos, const Index& lastpos)
{
	std::string rt;
	Index pi = firstpos > 5 ? (firstpos - 5):1, pe = lastpos + 5;
	for (; pi < pe; ++pi)
	{
		if (pi == firstpos)
		{
			rt.append(" | ");
		}
		if (pi == lastpos)
		{
			rt.append(" | ");
		}
		if (m_forwardindex->skipPos(pi) == pi)
		{
			if (!rt.empty()) rt.push_back(' ');
			rt.append( m_forwardindex->fetch());
		}
	}
	return rt;
}

SummarizerFunctionContextMatchPhrase::Match SummarizerFunctionContextMatchPhrase::logFindBestMatch_(
			std::ostream& out,
			WeightingData& wdata,
			unsigned int cardinality,
			PostingIteratorInterface** itrar)
{
	Match rt;
	Index lastEndPos = 0;
	unsigned int lastElementCnt = 0;
	Index firstpos = wdata.titleend;
	PositionWindow poswin( itrar, m_itrarsize, m_parameter->m_windowsize, cardinality,
				firstpos, PositionWindow::MaxWin);
	bool more = poswin.first();
	for (;more; more = poswin.next())
	{
		// Check if the window is a really new one and not one already covered:
		Index windowpos = poswin.pos();
		Index windowspan = poswin.span();
		if (lastEndPos == windowpos + windowspan && lastElementCnt > poswin.size())
		{
			continue;
		}
		lastElementCnt = poswin.size();
		lastEndPos = windowpos + windowspan;

		// Check if window is overlapping a paragraph. In this case to not use it for summary:
		std::pair<Index,Index> paraframe = wdata.paraiter.skipPos( windowpos);
		if (paraframe.first && paraframe.second < windowpos + windowspan) continue;

		// Calculate sentence frame:
		std::pair<Index,Index> structframe = wdata.structiter.skipPos( windowpos);

		// Calculate the candidate weight:
		double weight = windowWeight( wdata, poswin, structframe, paraframe);

		std::string candidatestr( getPhraseString( windowpos, windowpos + windowspan));
		out << string_format( _TXT( "candidate pos=%u, best=%u, span=%u, weight=%f, string=%s"),
					windowpos, (unsigned int)(weight > rt.weight), windowspan,
					weight, candidatestr.c_str()) << std::endl;

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

SummarizerFunctionContextMatchPhrase::Match
	SummarizerFunctionContextMatchPhrase::findBestMatch( WeightingData& wdata)
{
	return findBestMatch_( wdata, m_cardinality, wdata.valid_itrar);
}


void SummarizerFunctionContextMatchPhrase::fetchNoTitlePostings(
	WeightingData& wdata, PostingIteratorInterface** noTitle_itrar, Index& cntTitleTerms, Index& cntNoTitleTerms)
{
	cntTitleTerms = 0;
	cntNoTitleTerms = 0;
	std::size_t ti=0,te=m_itrarsize;
	for (; ti < te; ++ti)
	{
		if (wdata.valid_itrar[ti])
		{
			Index pos = wdata.valid_itrar[ti]->skipPos( wdata.titlestart);
			if (pos >= wdata.titleend)
			{
				noTitle_itrar[ ti] = wdata.valid_itrar[ ti];
				++cntNoTitleTerms;
			}
			else
			{
				noTitle_itrar[ ti] = 0;
				++cntTitleTerms;
			}
		}
		else
		{
			noTitle_itrar[ ti] = 0;
		}
	}
}

SummarizerFunctionContextMatchPhrase::Match
	SummarizerFunctionContextMatchPhrase::findBestMatchNoTitle( WeightingData& wdata)
{
	PostingIteratorInterface* noTitle_itrar[ MaxNofArguments];
	Index cntTitleTerms = 0;
	Index cntNoTitleTerms = 0;
	fetchNoTitlePostings( wdata, noTitle_itrar, cntTitleTerms, cntNoTitleTerms);
	if (cntNoTitleTerms)
	{
		unsigned int noTitleCardinality = m_cardinality > (unsigned int)cntTitleTerms ? (m_cardinality - cntTitleTerms) : 1;
		return findBestMatch_( wdata, noTitleCardinality, noTitle_itrar);
	}
	else
	{
		return Match();
	}
}

SummarizerFunctionContextMatchPhrase::Match
	SummarizerFunctionContextMatchPhrase::logFindBestMatchNoTitle( std::ostream& out, WeightingData& wdata)
{
	PostingIteratorInterface* noTitle_itrar[ MaxNofArguments];
	Index cntTitleTerms = 0;
	Index cntNoTitleTerms = 0;
	fetchNoTitlePostings( wdata, noTitle_itrar, cntTitleTerms, cntNoTitleTerms);
	if (cntNoTitleTerms)
	{
		unsigned int noTitleCardinality = (Index)m_cardinality > cntTitleTerms ? (m_cardinality - cntTitleTerms) : 1;
		return logFindBestMatch_( out, wdata, noTitleCardinality, noTitle_itrar);
	}
	else
	{
		return Match();
	}
}

SummarizerFunctionContextMatchPhrase::Match
	SummarizerFunctionContextMatchPhrase::findAbstractMatch( WeightingData& wdata)
{
	Match rt = findBestMatch( wdata);
	if (!rt.isDefined() && m_titleitr)
	{
		//... we did not find a window with m_cardinality terms, so we try to find one
		//	without the terms appearing in the document title:
		rt = findBestMatchNoTitle( wdata);
	}
	if (!rt.isDefined())
	{
		// ... if we did not find any window, we take the start of the document as abstract:
		rt.is_docstart = true;
		rt.pos = wdata.titleend;
		rt.span = m_parameter->m_sentencesize;
	}
	return rt;
}

SummarizerFunctionContextMatchPhrase::Match
	SummarizerFunctionContextMatchPhrase::logFindAbstractMatch( std::ostream& out, WeightingData& wdata)
{
	out << _TXT("find best match with all features:") << std::endl;
	Match rt = logFindBestMatch_( out, wdata, m_cardinality, wdata.valid_itrar);
	if (!rt.isDefined() && m_titleitr)
	{
		//... we did not find a window with m_cardinality terms, so we try to find one
		//	without the terms appearing in the document title:
		out << _TXT("find best match without title features:") << std::endl;
		rt = logFindBestMatchNoTitle( out, wdata);
	}
	if (!rt.isDefined())
	{
		// ... if we did not find any window, we take the start of the document as abstract:
		rt.is_docstart = true;
		rt.pos = wdata.titleend;
		rt.span = m_parameter->m_sentencesize;
		out << string_format( _TXT( "candidate begin doc, span=%u"), (unsigned int)rt.span) << std::endl;
		
	}
	return rt;
}

SummarizerFunctionContextMatchPhrase::Abstract
	SummarizerFunctionContextMatchPhrase::getPhraseAbstract( const Match& candidate, WeightingData& wdata)
{
	Abstract rt( candidate.pos, candidate.span, (candidate.pos == wdata.titleend), false, candidate.is_docstart);

	// Find the start of the abstract as start of the preceeding structure or paragraph marker:
	std::size_t ti=0,te=m_paraarsize+m_structarsize;
	Index astart;
	if ((Index)m_parameter->m_sentencesize + wdata.titleend < candidate.pos)
	{
		astart = candidate.pos - (Index)m_parameter->m_sentencesize;
	}
	else
	{
		astart = wdata.titleend;
		rt.defined_start = true;
	}
	for (; ti<te && astart < candidate.pos; ++ti)
	{
		if (wdata.valid_structar[ti])
		{
			Index pos = wdata.valid_structar[ti]->skipPos( astart);
			while (pos >= astart && pos <= candidate.pos)
			{
				rt.defined_start = true;
				astart = pos + 1;
				pos = wdata.valid_structar[ti]->skipPos( astart);
			}
		}
	}
	rt.span += candidate.pos - astart;
	rt.start = astart;

	// Find the end of the abstract, by scanning for the next sentence:
	if (rt.span < (Index)m_parameter->m_sentencesize)
	{
		Index minincr = 0;
		if (rt.span < ((Index)m_parameter->m_sentencesize >> 2))
		{
			minincr += (Index)m_parameter->m_sentencesize >> 2;
			// .... heuristics for minimal size of abstract we want to show
		}
		Index nextparapos = callSkipPos( rt.start + rt.span, wdata.valid_paraar, m_paraarsize);
		Index eospos = callSkipPos( rt.start + rt.span + minincr, wdata.valid_structar, m_structarsize + m_paraarsize);
		if (eospos)
		{
			if (nextparapos && nextparapos <= eospos) eospos = nextparapos - 1;
			if ((eospos - rt.start) < (Index)m_parameter->m_sentencesize)
			{
				rt.span = eospos - rt.start + 1;
				rt.defined_end = true;
			}
			else
			{
				rt.span = m_parameter->m_sentencesize;
			}
		}
	}
	return rt;
}

SummarizerFunctionContextMatchPhrase::Abstract
	SummarizerFunctionContextMatchPhrase::getParaTitleAbstract(
		Match& phrase_match,
		WeightingData& wdata)
{
	Abstract rt;

	Index searchrange = 300;
	Index parapos = 0;
	Index eoppos = 0;

	for (;;)
	{
		Index pstart = phrase_match.pos > (searchrange + wdata.titleend)
				? (phrase_match.pos - searchrange)
				: (wdata.titleend + 1);

		Index prevparapos = callSkipPos( pstart, wdata.valid_paraar, m_paraarsize);
		if (prevparapos && prevparapos <= phrase_match.pos)
		{
			parapos = prevparapos;
			for (;;)
			{
				prevparapos = callSkipPos( parapos+1, wdata.valid_paraar, m_paraarsize);
				if (prevparapos && prevparapos <= phrase_match.pos)
				{
					parapos = prevparapos;
				}
				else
				{
					break;
				}
			}
			eoppos = callSkipPos( parapos+1, wdata.valid_structar, m_structarsize);
			if (eoppos && eoppos - parapos < MaxParaTitleSize)
			{
				if (eoppos >= phrase_match.pos)
				{
					// ... paragraph title is overlapping with best match
					if (eoppos < phrase_match.pos + phrase_match.span)
					{
						phrase_match.span -= (eoppos - phrase_match.pos);
						phrase_match.pos = eoppos;
					}
					else
					{
						phrase_match.span = m_parameter->m_sentencesize >> 2;
						phrase_match.pos = eoppos;
					}
				}
				rt.start = parapos;
				rt.span = eoppos-parapos+1;
			}
			break;
		}
		if (phrase_match.pos <= (searchrange + wdata.titleend))
		{
			break;
		}
		searchrange *= 2;
	}
	return rt;
}

std::string SummarizerFunctionContextMatchPhrase::getParaTitleString( const Abstract& para_abstract)
{
	std::string rt;
	Index pi = para_abstract.start, pe = para_abstract.start + para_abstract.span;
	for (; pi < pe; ++pi)
	{
		if (m_forwardindex->skipPos(pi) == pi)
		{
			if (!rt.empty()) rt.push_back(' ');
			rt.append( m_forwardindex->fetch());
		}
	}
	return rt;
}

std::string SummarizerFunctionContextMatchPhrase::getPhraseString( const Abstract& phrase_abstract, WeightingData& wdata)
{
	std::string rt;
	// Create the array of positions to highlight:
	std::vector<Index> highlightpos;
	Index pi = phrase_abstract.start, pe = phrase_abstract.start + phrase_abstract.span;
	while (pi < pe)
	{
		std::pair<Index,Index> minpos = callSkipPosWithLen( pi, wdata.valid_itrar, m_itrarsize);
		if (minpos.first && minpos.first < pe)
		{
			Index li = 0, le = minpos.second;
			for (; li != le; ++li)
			{
				highlightpos.push_back( minpos.first+li);
			}
			pi = minpos.first+(le?le:1);
		}
		else
		{
			break;
		}
	}
	// Build the result (phrase):
	if (!phrase_abstract.defined_start)
	{
		rt.append( m_parameter->m_floatingmark.first);
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
			if (!rt.empty()) rt.push_back(' ');
			for (; hi < he && pi > highlightpos[hi]; ++hi){}
			if (hi < he && pi == highlightpos[hi])
			{
				rt.append( m_parameter->m_matchmark.first);
				rt.append( m_forwardindex->fetch());
				rt.append( m_parameter->m_matchmark.second);
			}
			else
			{
				rt.append( m_forwardindex->fetch());
			}
		}
	}
	if (!phrase_abstract.defined_end)
	{
		rt.append( m_parameter->m_floatingmark.second);
	}
	return rt;
}

std::vector<SummaryElement>
	SummarizerFunctionContextMatchPhrase::getSummariesFromAbstracts(
		const Abstract& para_abstract,
		const Abstract& phrase_abstract,
		WeightingData& wdata)
{
	std::vector<SummaryElement> rt;

	// Create the paragraph result, if exists:
	if (para_abstract.isDefined())
	{
		std::string paratitle = getParaTitleString( para_abstract);
		rt.push_back( SummaryElement( m_parameter->m_name_para, paratitle, 1.0));
	}
	// Create the highlighted phrase result, if exists:
	if (phrase_abstract.isDefined())
	{
		std::string phrase = getPhraseString( phrase_abstract, wdata);
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

void SummarizerFunctionContextMatchPhrase::initializeContext()
{
	{
		// initialize proportional ff increment weights
		m_weightincr.init( m_itrarsize);
		ProximityWeightAccumulator::proportionalAssignment( m_weightincr, 1.0, m_parameter->m_prop_weight_const, m_idfar);

		if (m_cardinality == 0)
		{
			if (m_parameter->m_cardinality_frac > std::numeric_limits<double>::epsilon())
			{
				m_cardinality = std::max( 1U, (unsigned int)(m_itrarsize * m_parameter->m_cardinality_frac + 0.5));
			}
			else
			{
				m_cardinality = m_itrarsize;
			}
		}
		if (m_nof_maxdf_features >= m_cardinality && m_nof_maxdf_features < m_itrarsize)
		{
			m_cardinality = m_nof_maxdf_features+1;
			//... at least on feature in the windows visited must fulfill the maxdf criterion
		}
		m_initialized = true;
	}
}

void SummarizerFunctionContextMatchPhrase::initWeightingData( WeightingData& wdata, const Index& docno)
{
	callSkipDoc( docno, m_itrar, m_itrarsize, wdata.valid_itrar);
	callSkipDoc( docno, m_structar, m_structarsize + m_paraarsize, wdata.valid_structar);

	if (m_titleitr && m_titleitr->skipDoc( docno) == docno)
	{
		wdata.titlestart = m_titleitr->skipPos(0);
		if (wdata.titlestart)
		{
			Index ti = wdata.titleend = wdata.titlestart;
			while (0!=(ti=m_titleitr->skipPos(ti+1)))
			{
				wdata.titleend = ti;
			}
			++wdata.titleend;
		}
		else
		{
			wdata.titlestart = 1;
		}
	}
}

std::vector<SummaryElement>
	SummarizerFunctionContextMatchPhrase::getSummary( const Index& docno)
{
	try
	{
		if (m_itrarsize == 0)
		{
			return std::vector<SummaryElement>();
		}
		if (!m_initialized)
		{
			initializeContext();
		}
		if (m_itrarsize < m_cardinality)
		{
			return std::vector<SummaryElement>();
		}
		// Init document iterators:
		WeightingData wdata( m_structarsize, m_paraarsize, m_parameter->m_sentencesize, m_parameter->m_paragraphsize);
		initWeightingData( wdata, docno);

		m_forwardindex->skipDoc( docno);

		Index firstpos = m_forwardindex->skipPos( 0);
		if (!firstpos) return std::vector<SummaryElement>();

		// Find the match for abstract:
		Match candidate = findAbstractMatch( wdata);

		// Get the title of the paragraph the best phrase belongs to:
		Abstract para_abstract = getParaTitleAbstract( candidate, wdata);
		// Get the best phrase
		Abstract phrase_abstract = getPhraseAbstract( candidate, wdata);

		return getSummariesFromAbstracts( para_abstract, phrase_abstract, wdata);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summary: %s"), METHOD_NAME, *m_errorhnd, std::vector<SummaryElement>());
}

std::string SummarizerFunctionContextMatchPhrase::debugCall( const Index& docno)
{
	std::ostringstream out;
	out << std::fixed << std::setprecision(8);
	out << string_format( _TXT( "summarize %s"), METHOD_NAME) << std::endl;

	if (m_itrarsize == 0)
	{
		return std::string();
	}
	if (!m_initialized)
	{
		initializeContext();
	}
	if (m_itrarsize < m_cardinality)
	{
		return std::string();
	}
	// Init document iterators:
	WeightingData wdata( m_structarsize, m_paraarsize, m_parameter->m_sentencesize, m_parameter->m_paragraphsize);
	initWeightingData( wdata, docno);

	m_forwardindex->skipDoc( docno);

	Index firstpos = m_forwardindex->skipPos( 0);
	if (!firstpos) return std::string();

	Match candidate = logFindAbstractMatch( out, wdata);
	Abstract phrase_abstract = getPhraseAbstract( candidate, wdata);
	Abstract para_abstract = getParaTitleAbstract( candidate, wdata);

	out << string_format( _TXT("best match pos=%u, span=%u, is_docstart=%u"),
				candidate.pos, candidate.span, candidate.is_docstart) << std::endl;

	std::string phrasestr = getPhraseString( phrase_abstract, wdata);
	out << string_format( _TXT("best match phrase pos=%u, span=%u, has start %u, has end=%u, string='%s'"),
				phrase_abstract.start, phrase_abstract.span, phrase_abstract.defined_start,
				phrase_abstract.defined_end, phrasestr.c_str()) << std::endl;

	if (para_abstract.isDefined())
	{
		std::string titlestr = getParaTitleString( para_abstract);
		out << string_format( _TXT("best match paragraph pos=%u, span=%u, string='%s'"),
					para_abstract.start, para_abstract.span, titlestr.c_str()) << std::endl;
	}
	return out.str();
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
		if (strus::caseInsensitiveEquals( name, "match") || strus::caseInsensitiveEquals( name, "struct") || strus::caseInsensitiveEquals( name, "para") || strus::caseInsensitiveEquals( name, "title"))
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as string"), name.c_str(), "matchvariables");
		}
		else if (strus::caseInsensitiveEquals( name, "type"))
		{
			m_parameter->m_type = value;
		}
		else if (strus::caseInsensitiveEquals( name, "matchmark"))
		{
			m_parameter->m_matchmark = parseMarker( value);
		}
		else if (strus::caseInsensitiveEquals( name, "floatingmark"))
		{
			m_parameter->m_floatingmark = parseMarker( value);
		}
		else if (strus::caseInsensitiveEquals( name, "sentencesize")
			|| strus::caseInsensitiveEquals( name, "paragraphsize")
			|| strus::caseInsensitiveEquals( name, "windowsize"))
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("no string value expected for parameter '%s' in summarization function '%s'"), name.c_str(), METHOD_NAME);
		}
		else if (strus::caseInsensitiveEquals( name, "cardinality") && !value.empty() && value[value.size()-1] == '%')
		{
			m_parameter->m_cardinality = 0;
			m_parameter->m_cardinality_frac = numstring_conv::todouble( value);
		}
		else
		{
			m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' summarization function parameter '%s'"), METHOD_NAME, name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding string parameter to '%s' summarizer: %s"), METHOD_NAME, *m_errorhnd);
}

void SummarizerFunctionInstanceMatchPhrase::addNumericParameter( const std::string& name, const NumericVariant& value)
{
	if (strus::caseInsensitiveEquals( name, "match") || strus::caseInsensitiveEquals( name, "struct") || strus::caseInsensitiveEquals( name, "para") || strus::caseInsensitiveEquals( name, "title"))
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as numeric value"), name.c_str(), METHOD_NAME);
	}
	else if (strus::caseInsensitiveEquals( name, "paragraphsize"))
	{
		m_parameter->m_paragraphsize = value.touint();
	}
	else if (strus::caseInsensitiveEquals( name, "sentencesize"))
	{
		m_parameter->m_sentencesize = value.touint();
	}
	else if (strus::caseInsensitiveEquals( name, "windowsize"))
	{
		m_parameter->m_windowsize = value.touint();
	}
	else if (strus::caseInsensitiveEquals( name, "cardinality"))
	{
		m_parameter->m_cardinality = value.touint();
		m_parameter->m_cardinality_frac = 0.0;
	}
	else if (strus::caseInsensitiveEquals( name, "maxdf"))
	{
		m_parameter->m_maxdf = (double)value;
		if (m_parameter->m_maxdf < 0.0 || m_parameter->m_maxdf > 1.0)
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for weighting scheme '%s' expected to a positive floating point number between 0.0 and 1.0"), name.c_str(), METHOD_NAME);
		}
	}
	else if (strus::caseInsensitiveEquals( name, "type")
		|| strus::caseInsensitiveEquals( name, "matchmark")
		|| strus::caseInsensitiveEquals( name, "floatingmark")
		|| strus::caseInsensitiveEquals( name, "metadata_title_maxpos"))
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("no numeric value expected for parameter '%s' in summarization function '%s'"), name.c_str(), METHOD_NAME);
	}
	else
	{
		m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' summarization function parameter '%s'"), METHOD_NAME, name.c_str());
	}
}

void SummarizerFunctionInstanceMatchPhrase::defineResultName(
		const std::string& resultname,
		const std::string& itemname)
{
	try
	{
		if (strus::caseInsensitiveEquals( itemname, "para"))
		{
			m_parameter->m_name_para = resultname;
		}
		else if (strus::caseInsensitiveEquals( itemname, "phrase"))
		{
			m_parameter->m_name_phrase = resultname;
		}
		else if (strus::caseInsensitiveEquals( itemname, "docstart"))
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
		m_errorhnd->report( ErrorCodeIncompleteDefinition, _TXT( "emtpy term type definition (parameter 'type') in match phrase summarizer configuration"));
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
			<< ", name para=" << m_parameter->m_name_para
			<< ", name phrase=" << m_parameter->m_name_phrase
			<< ", name docstart=" << m_parameter->m_name_docstart
			<< ", paragraphsize=" << m_parameter->m_paragraphsize
			<< ", sentencesize=" << m_parameter->m_sentencesize
			<< ", windowsize=" << m_parameter->m_windowsize;
			if (m_parameter->m_cardinality_frac > std::numeric_limits<double>::epsilon())
			{
				rt << ", cardinality='" << (unsigned int)(m_parameter->m_cardinality_frac * 100 + 0.5) << "%'";
			}
			else
			{
				rt << ", cardinality='" << m_parameter->m_cardinality << "'";
			}
			rt << ", maxdf='" << m_parameter->m_maxdf << "'";
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
		rt( P::Feature, "title", _TXT( "defines the title field of documents"), "");
		rt( P::String, "type", _TXT( "the forward index type of the result phrase elements"), "");
		rt( P::Numeric, "paragraphsize", _TXT( "estimated size of a paragraph"), "1:");
		rt( P::Numeric, "sentencesize", _TXT( "estimated size of a sentence, also a restriction for the maximum length of sentences in summaries"), "1:");
		rt( P::Numeric, "windowsize", _TXT( "maximum size of window used for identifying matches"), "1:");
		rt( P::Numeric, "cardinality", _TXT( "minimum number of features in a window"), "1:");
		rt( P::Numeric, "maxdf", _TXT( "the maximum df (fraction of collection size) of features considered for same sentence proximity weighing"), "1:");
		rt( P::String, "matchmark", _TXT( "specifies the markers (first character of the value is the separator followed by the two parts separated by it) for highlighting matches in the resulting phrases"), "");
		rt( P::String, "floatingmark", _TXT( "specifies the markers (first character of the value is the separator followed by the two parts separated by it) for marking floating phrases without start or end of sentence found"), "");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating summarizer function description for '%s': %s"), METHOD_NAME, *m_errorhnd, FunctionDescription());
}

