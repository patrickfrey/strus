/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "summarizerAccumulateNear.hpp"
#include "proximityWeightAccumulator.hpp"
#include "positionWindow.hpp"
#include "postingIteratorLink.hpp"
#include "ranker.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/constants.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/string_conv.hpp"
#include "strus/base/numstring.hpp"
#include "strus/base/math.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/functionDescription.hpp"
#include <limits>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace strus;

#define THIS_METHOD_NAME const_cast<char*>("matchnear")

SummarizerFunctionContextAccumulateNear::SummarizerFunctionContextAccumulateNear(
		const StorageClientInterface* storage_,
		const QueryProcessorInterface* processor_,
		const Reference<AccumulateNearData>& data_,
		double nofCollectionDocuments_,
		ErrorBufferInterface* errorhnd_)
	:m_storage(storage_)
	,m_processor(processor_)
	,m_forwardindex(storage_->createForwardIterator( data_->type))
	,m_data(data_)
	,m_nofCollectionDocuments(nofCollectionDocuments_)
	,m_idfar()
	,m_itrarsize(0)
	,m_structarsize(0)
	,m_cardinality(data_->cardinality)
	,m_minwinsize(1)
	,m_weightincr()
	,m_initialized(false)
	,m_errorhnd(errorhnd_)
{
	if (!m_forwardindex.get()) throw std::runtime_error( _TXT("error creating forward index iterator"));
}

void SummarizerFunctionContextAccumulateNear::setVariableValue( const std::string&, double)
{
	m_errorhnd->report( ErrorCodeNotImplemented, _TXT("no variables known for function '%s'"), THIS_METHOD_NAME);
}

void SummarizerFunctionContextAccumulateNear::addSummarizationFeature(
		const std::string& name_,
		PostingIteratorInterface* itr,
		const std::vector<SummarizationVariable>& variables,
		double weight,
		const TermStatistics& termstats)
{
	try
	{
		if (strus::caseInsensitiveEquals( name_, "struct"))
		{
			if (m_structarsize > MaxNofArguments) throw strus::runtime_error( "%s",  _TXT("number of structure features out of range"));
			m_structar[ m_structarsize++] = itr;
		}
		else if (strus::caseInsensitiveEquals( name_, "match"))
		{
			if (m_itrarsize > MaxNofArguments) throw strus::runtime_error( "%s",  _TXT("number of weighting features out of range"));

			double df = termstats.documentFrequency()>=0?termstats.documentFrequency():(GlobalCounter)itr->documentFrequency();
			double idf = strus::Math::log( (m_nofCollectionDocuments - df + 0.5) / (df + 0.5));
			if (idf < 0.00001)
			{
				idf = 0.00001;
			}
			m_itrar[ m_itrarsize++] = itr;
			m_idfar.push( idf * weight);
		}
		else
		{
			m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' summarization function parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding feature to '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd);
}

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

void SummarizerFunctionContextAccumulateNear::initializeContext()
{
	if (m_cardinality == 0)
	{
		if (m_data->cardinality_frac > std::numeric_limits<double>::epsilon())
		{
			m_cardinality = std::max( 1U, (unsigned int)(m_itrarsize * m_data->cardinality_frac + 0.5));
		}
		else
		{
			m_cardinality = m_itrarsize;
		}
	}
	// initialize proportional ff increment weights
	m_weightincr.init( m_itrarsize);
	ProximityWeightAccumulator::proportionalAssignment( m_weightincr, 1.0, m_data->cprop, m_idfar);

	double factor = 1.0;
	for (std::size_t ii=0; ii<m_itrarsize; ++ii)
	{
		m_normfactorar[ ii] = factor / sqrt(m_itrarsize - ii);
		factor *= m_data->cofactor;
	}
	m_minwinsize = m_cardinality ? m_cardinality : m_itrarsize;
	if (m_minwinsize > m_itrarsize) m_minwinsize = m_itrarsize;

	m_initialized = true;
}

bool SummarizerFunctionContextAccumulateNear::getCandidateEntity( CandidateEntity& res, const PositionWindow& poswin, PostingIteratorInterface** valid_itrar, PostingIteratorInterface** valid_structar)
{
	Index windowpos = poswin.pos();
	res.window = poswin.window();
	res.windowsize = poswin.size();
	res.windowendpos = valid_itrar[ res.window[ m_minwinsize-1]]->posno();

	Index startpos = res.windowendpos < m_data->range ? 0 :(res.windowendpos - m_data->range);
	Index struct_startpos = callSkipPos( startpos, valid_structar, m_structarsize);
	res.structpos = 0;
	if (!struct_startpos || struct_startpos > windowpos)
	{
		res.structpos = struct_startpos;
	}
	else
	{
		res.structpos = callSkipPos( windowpos, valid_structar, m_structarsize);
	}
	if (!res.structpos || res.structpos > windowpos + m_data->range)
	{
		res.structpos = windowpos + m_data->range + 1;
	}
	res.forwardpos = m_forwardindex->skipPos( startpos);
	if (res.forwardpos && res.forwardpos < res.structpos && res.windowendpos < res.structpos)
	{
		// Calculate the window size not overlapping with :
		Index windowspan = poswin.span();
		Index windowmaxpos = windowpos + windowspan;
		if (res.structpos >= windowmaxpos)
		{
			res.windowendpos = windowmaxpos;
		}
		else for (std::size_t wi=m_minwinsize; wi<res.windowsize; ++wi)
		{
			Index wpos = valid_itrar[ res.window[ wi]]->posno();
			if (wpos < res.structpos)
			{
				res.windowendpos = wpos;
			}
			else
			{
				res.windowsize = wi;
				break;
			}
		}
		return true;
	}
	return false;
}

double SummarizerFunctionContextAccumulateNear::candidateWeight( const CandidateEntity& candidate, PostingIteratorInterface** valid_itrar) const
{
	double rt = 0.0;
	for (std::size_t wi=0; wi<candidate.windowsize; ++wi)
	{
		double distweight = 1.0 / strus::Math::sqrt( strus::Math::abs( candidate.forwardpos - valid_itrar[ candidate.window[ wi]]->posno()) + DIST_WEIGHT_BASE);
		rt += m_weightincr[ candidate.window[ wi]] * distweight;
	}
	return rt;
}


void SummarizerFunctionContextAccumulateNear::initEntityMap( EntityMap& entitymap, const Index& docno)
{
	// Initialize posting iterators
	PostingIteratorInterface* valid_itrar[ MaxNofArguments];	//< valid array if weighted features
	PostingIteratorInterface* valid_structar[ MaxNofArguments];	//< valid array of end of structure elements
	
	callSkipDoc( docno, m_itrar, m_itrarsize, valid_itrar);
	callSkipDoc( docno, m_structar, m_structarsize, valid_structar);
	m_forwardindex->skipDoc( docno);

	// Fetch entities and weight them:
	PositionWindow poswin( valid_itrar, m_itrarsize, m_data->range, m_cardinality,
				0, PositionWindow::MaxWin);
	bool more = poswin.first();
	while (more)
	{
		Index nextstart;
		CandidateEntity candidate;
		if (getCandidateEntity( candidate, poswin, valid_itrar, valid_structar))
		{
			double normfactor = m_normfactorar[ candidate.windowsize-1];

			// Calculate the weights of matching entities:
			while (candidate.forwardpos && candidate.forwardpos < candidate.structpos)
			{
				double ww = candidateWeight( candidate, valid_itrar);
				entitymap[ m_forwardindex->fetch()] += normfactor * ww;
				candidate.forwardpos = m_forwardindex->skipPos( candidate.forwardpos + 1);
			}
			nextstart = candidate.windowendpos;
		}
		else
		{
			nextstart = valid_itrar[ candidate.window[ m_minwinsize-1]]->posno();
		}
		more = poswin.skip( nextstart);
	}
}

std::vector<SummaryElement>
	SummarizerFunctionContextAccumulateNear::getSummariesFromEntityMap( EntityMap& entitymap) const
{
	std::vector<SummaryElement> rt;

	Ranker ranker( m_data->nofranks);
	EntityMap::const_iterator ei = entitymap.begin(), ee = entitymap.end();
	std::vector<EntityMap::const_iterator> valuerefs;
	for (; ei != ee; ++ei)
	{
		ranker.insert( ei->second / m_data->norm, valuerefs.size());
		valuerefs.push_back( ei);
	}
	std::vector<Ranker::Element> result = ranker.result();
	std::vector<Ranker::Element>::const_iterator ri = result.begin(), re = result.end();
	for (; ri != re; ++ri)
	{
		rt.push_back( SummaryElement( m_data->resultname, valuerefs[ ri->idx]->first, ri->weight));
	}
	return rt;
}

std::vector<SummaryElement>
	SummarizerFunctionContextAccumulateNear::getSummary( const Index& docno)
{
	try
	{
		// Initialization:
		if (m_itrarsize == 0)
		{
			return std::vector<SummaryElement>();
		}
		if (!m_initialized) initializeContext();
		if (m_itrarsize < m_cardinality)
		{
			return std::vector<SummaryElement>();
		}

		// Init map of weighted entities:
		EntityMap entitymap;
		initEntityMap( entitymap, docno);

		// Get summary from map of weighted entities
		return getSummariesFromEntityMap( entitymap);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summary: %s"), THIS_METHOD_NAME, *m_errorhnd, std::vector<SummaryElement>());
}


std::string SummarizerFunctionContextAccumulateNear::debugCall( const Index& docno)
{
	std::ostringstream out;
	out << std::fixed << std::setprecision(8);
	out << string_format( _TXT( "summarize %s"), THIS_METHOD_NAME) << std::endl;

	// Initialization:
	if (m_itrarsize == 0)
	{
		return std::string();
	}
	if (!m_initialized) initializeContext();
	if (m_itrarsize < m_cardinality)
	{
		return std::string();
	}

	// Initialize posting iterators
	PostingIteratorInterface* valid_itrar[ MaxNofArguments];	//< valid array if weighted features
	PostingIteratorInterface* valid_structar[ MaxNofArguments];	//< valid array of end of structure elements
	
	callSkipDoc( docno, m_itrar, m_itrarsize, valid_itrar);
	callSkipDoc( docno, m_structar, m_structarsize, valid_structar);
	m_forwardindex->skipDoc( docno);

	// Fetch entities and print them with weight:
	PositionWindow poswin( valid_itrar, m_itrarsize, m_data->range, m_cardinality,
				0, PositionWindow::MaxWin);
	bool more = poswin.first();
	while (more)
	{
		Index nextstart;
		CandidateEntity candidate;
		if (getCandidateEntity( candidate, poswin, valid_itrar, valid_structar))
		{
			double normfactor = m_normfactorar[ candidate.windowsize-1];

			// Calculate the weights of matching entities:
			while (candidate.forwardpos && candidate.forwardpos < candidate.structpos)
			{
				double ww = normfactor * candidateWeight( candidate, valid_itrar);
				std::string keystr( m_forwardindex->fetch());
				out << string_format( _TXT( "entity pos=%u, span=%u, weight=%f, value='%s'"),
							poswin.pos(), poswin.span(), ww, keystr.c_str()) << std::endl;
			}
			nextstart = candidate.windowendpos;
		}
		else
		{
			nextstart = valid_itrar[ candidate.window[ m_minwinsize-1]]->posno();
		}
		more = poswin.skip( nextstart);
	}
	return out.str();
}

void SummarizerFunctionInstanceAccumulateNear::addStringParameter( const std::string& name_, const std::string& value)
{
	try
	{
		if (strus::caseInsensitiveEquals( name_, "match") || strus::caseInsensitiveEquals( name_, "struct"))
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as string"), name_.c_str(), THIS_METHOD_NAME);
		}
		else if (strus::caseInsensitiveEquals( name_, "type"))
		{
			m_data->type = value;
			if (m_data->resultname.empty())
			{
				m_data->resultname = value;
			}
		}
		else if (strus::caseInsensitiveEquals( name_, "result"))
		{
			m_data->resultname = value;
		}
		else if (strus::caseInsensitiveEquals( name_, "cardinality") && !value.empty() && value[value.size()-1] == '%')
		{
			m_data->cardinality = 0;
			m_data->cardinality_frac = numstring_conv::todouble( value);
		}
		else if (strus::caseInsensitiveEquals( name_, "cofactor")
			|| strus::caseInsensitiveEquals( name_, "norm")
			|| strus::caseInsensitiveEquals( name_, "cprop")
			|| strus::caseInsensitiveEquals( name_, "nofranks")
			|| strus::caseInsensitiveEquals( name_, "range"))
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for summarizer '%s' expected to be defined as string and not as numeric value"), name_.c_str(), THIS_METHOD_NAME);
		}
		else
		{
			throw strus::runtime_error( _TXT("unknown '%s' summarization function parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding string parameter to '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd);
}

void SummarizerFunctionInstanceAccumulateNear::addNumericParameter( const std::string& name_, const NumericVariant& value)
{
	if (strus::caseInsensitiveEquals( name_, "match") || strus::caseInsensitiveEquals( name_, "struct"))
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as numeric value"), name_.c_str(), THIS_METHOD_NAME);
	}
	else if (strus::caseInsensitiveEquals( name_, "type")
		|| strus::caseInsensitiveEquals( name_, "result"))
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("no numeric value expected for parameter '%s' in summarization function '%s'"), name_.c_str(), THIS_METHOD_NAME);
	}
	else if (strus::caseInsensitiveEquals( name_, "cofactor"))
	{
		m_data->cofactor = value.tofloat();
	}
	else if (strus::caseInsensitiveEquals( name_, "norm"))
	{
		m_data->cofactor = value.tofloat();
	}
	else if (strus::caseInsensitiveEquals( name_, "cprop"))
	{
		m_data->cprop = value.tofloat();
		if (m_data->cprop < 0.0 || m_data->cprop > 1.0)
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for summarizer '%s' expected to be a floating point number between 0 and 1"), name_.c_str(), THIS_METHOD_NAME);
		}
	}
	else if (strus::caseInsensitiveEquals( name_, "range"))
	{
		m_data->range = value.touint();
	}
	else if (strus::caseInsensitiveEquals( name_, "nofranks"))
	{
		m_data->nofranks = value.touint();
	}
	else if (strus::caseInsensitiveEquals( name_, "cardinality"))
	{
		m_data->cardinality = value.touint();
		m_data->cardinality_frac = 0.0;
	}
	else
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("unknown '%s' summarization function parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
	}
}

void SummarizerFunctionInstanceAccumulateNear::defineResultName(
		const std::string& resultname,
		const std::string& itemname)
{
	try
	{
		throw strus::runtime_error(_TXT("no result rename defined for '%s' summarizer"), THIS_METHOD_NAME);
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error defining result name of '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd);
}

SummarizerFunctionContextInterface* SummarizerFunctionInstanceAccumulateNear::createFunctionContext(
		const StorageClientInterface* storage,
		MetaDataReaderInterface*,
		const GlobalStatistics& stats) const
{
	if (m_data->type.empty())
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT( "empty forward index type definition (parameter 'type') in match phrase summarizer configuration"));
	}
	try
	{
		double nofCollectionDocuments = stats.nofDocumentsInserted()>=0?stats.nofDocumentsInserted():(GlobalCounter)storage->nofDocumentsInserted();
		return new SummarizerFunctionContextAccumulateNear( storage, m_processor, m_data, nofCollectionDocuments, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}

StructView SummarizerFunctionInstanceAccumulateNear::view() const
{
	try
	{
		StructView rt;
		rt( "type", m_data->type);
		rt( "result", m_data->resultname);
		rt( "cofactor", m_data->cofactor);
		rt( "nofranks", m_data->nofranks);
		if (m_data->cardinality_frac > std::numeric_limits<float>::epsilon())
		{
			rt( "cardinality", strus::string_format( "%u%%", (unsigned int)(m_data->cardinality_frac * 100 + 0.5)));
		}
		else
		{
			rt( "cardinality", m_data->cardinality);
		}
		rt( "range", m_data->range);
		rt( "norm", m_data->norm);
		rt( "cprop", m_data->cprop);
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summarizer introspection view: %s"), THIS_METHOD_NAME, *m_errorhnd, std::string());
}

SummarizerFunctionInstanceInterface* SummarizerFunctionAccumulateNear::createInstance(
		const QueryProcessorInterface* processor) const
{
	try
	{
		return new SummarizerFunctionInstanceAccumulateNear( processor, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}



StructView SummarizerFunctionAccumulateNear::view() const
{
	try
	{
		typedef FunctionDescription P;
		FunctionDescription rt( _TXT("Extract and weight all elements in the forward index of a given type that are within a window with features specified."));
		rt( P::Feature, "match", _TXT( "defines the query features to inspect for near matches"), "");
		rt( P::Feature, "struct", _TXT( "defines a structural delimiter for interaction of features on the same result"), "");
		rt( P::String, "type", _TXT( "the forward index feature type for the content to extract"), "");
		rt( P::String, "result", _TXT( "the name of the result if not equal to type"), "");
		rt( P::Numeric, "cofactor", _TXT( "multiplication factor for features pointing to the same result"), "");
		rt( P::Numeric, "norm", _TXT( "normalization factor for end result weights"), "");
		rt( P::Numeric, "nofranks", _TXT( "maximum number of ranks per document"), "");
		rt( P::Numeric, "cardinality", _TXT( "mimimum number of features per weighted item"), "");
		rt( P::Numeric, "range", _TXT( "maximum distance (ordinal position) of the weighted features (window size)"), "");
		rt( P::Numeric, "cprop", _TXT("constant part of idf proportional feature weight"), "0.0:1.0");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating summarizer function description for '%s': %s"), THIS_METHOD_NAME, *m_errorhnd, FunctionDescription());
}

