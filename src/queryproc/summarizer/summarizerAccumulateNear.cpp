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
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/utils.hpp"
#include <cstdlib>
#include <iostream>

using namespace strus;

#define METHOD_NAME "matchnear"

#undef STRUS_LOWLEVEL_DEBUG

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
	,m_weightincr()
	,m_initialized(false)
	,m_errorhnd(errorhnd_)
{
	if (!m_forwardindex.get()) throw strus::runtime_error(_TXT("error creating forward index iterator"));
}


void SummarizerFunctionContextAccumulateNear::addSummarizationFeature(
		const std::string& name,
		PostingIteratorInterface* itr,
		const std::vector<SummarizationVariable>& variables,
		double weight,
		const TermStatistics& termstats)
{
	try
	{
		if (utils::caseInsensitiveEquals( name, "struct"))
		{
			if (m_structarsize > MaxNofArguments) throw strus::runtime_error( _TXT("number of structure features out of range"));
			m_structar[ m_structarsize++] = itr;
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

std::vector<SummaryElement>
	SummarizerFunctionContextAccumulateNear::getSummary( const Index& docno)
{
	try
	{
		std::vector<SummaryElement> rt;
		std::map<std::string,double> entitymap;

		// Initialization:
		if (!m_initialized)
		{
			if (m_itrarsize == 0)
			{
				return rt;
			}
			if (m_data->cardinality == 0)
			{
				if (m_data->cardinality_frac > 0.0f)
				{
					m_data->cardinality = (unsigned int)(m_itrarsize * m_data->cardinality_frac + 0.5);
				}
				else
				{
					m_data->cardinality = m_itrarsize;
				}
			}
			if (m_itrarsize < m_data->cardinality)
			{
				return rt;
			}
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << string_format( "init summarizer %s: features %u, cardinality %u, range %u", METHOD_NAME, m_itrarsize, m_data->cardinality, m_data->range) << std::endl;
#endif
			// initialize proportional ff increment weights
			m_weightincr.init( m_itrarsize);
			ProximityWeightAccumulator::proportionalAssignment( m_weightincr, 1.0, 0.3, m_idfar);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "feature weights " <<  m_weightincr.tostring() << std::endl;
#endif
			double factor = 1.0;
			for (std::size_t ii=0; ii<m_itrarsize; ++ii)
			{
				m_normfactorar[ ii] = factor / sqrt(m_itrarsize - ii);
				factor *= m_data->cofactor;
#ifdef STRUS_LOWLEVEL_DEBUG
				std::cerr << string_format( "normfactor %u %f", ii, m_normfactorar[ ii]) << std::endl;
#endif
			}
			m_initialized = true;
		}

		// Init document iterators:
		PostingIteratorInterface* valid_itrar[ MaxNofArguments];			//< valid array if weighted features
		PostingIteratorInterface* valid_structar[ MaxNofArguments];			//< valid array of end of structure elements

		callSkipDoc( docno, m_itrar, m_itrarsize, valid_itrar);
		callSkipDoc( docno, m_structar, m_structarsize, valid_structar);
		m_forwardindex->skipDoc( docno);

		// Calculate weights:
		std::size_t minwinsize = m_data->cardinality ? m_data->cardinality : m_itrarsize;
		if (minwinsize > m_itrarsize) minwinsize = m_itrarsize;

		PositionWindow poswin( valid_itrar, m_itrarsize, m_data->range, m_data->cardinality,
					0, PositionWindow::MaxWin);
		bool more = poswin.first();
		while (more)
		{
			const std::size_t* window = poswin.window();
			std::size_t windowsize = poswin.size();
			Index windowpos = poswin.pos();
			Index windowendpos = valid_itrar[ window[ minwinsize-1]]->posno();

			Index startpos = windowendpos < m_data->range ? 0 :(windowendpos - m_data->range);
			Index struct_startpos = callSkipPos( startpos, valid_structar, m_structarsize);
			Index structpos = 0;
			if (!struct_startpos || struct_startpos > windowpos)
			{
				startpos = windowpos;
				structpos = struct_startpos;
			}
			else
			{
				structpos = callSkipPos( windowpos, valid_structar, m_structarsize);
			}
			Index forwardpos = m_forwardindex->skipPos( startpos);
			if (windowendpos < structpos)
			{
				// Calculate the window size not overlapping with :
				Index windowspan = poswin.span();
				Index windowmaxpos = windowpos + windowspan;
				if (structpos >= windowmaxpos)
				{
					windowendpos = windowmaxpos;
				}
				else for (std::size_t wi=minwinsize; wi<windowsize; ++wi)
				{
					Index wpos = valid_itrar[ window[ wi]]->posno();
					if (wpos < structpos)
					{
						windowendpos = wpos;
					}
					else
					{
						windowsize = wi;
						break;
					}
				}
				double normfactor = m_normfactorar[ windowsize];

				// Calculate the weights:
				while (forwardpos && forwardpos < structpos)
				{
					double weightsum = 0.0;
					for (std::size_t wi=0; wi<windowsize; ++wi)
					{
						weightsum += m_weightincr[ window[ wi]];
					}
					entitymap[ m_forwardindex->fetch()] += normfactor * weightsum;
					forwardpos = m_forwardindex->skipPos( forwardpos + 1);
				}
			}
			more = poswin.skip( windowendpos);
		}
		Ranker ranker( m_data->nofranks);
		std::map<std::string,double>::const_iterator ei = entitymap.begin(), ee = entitymap.end();
		std::vector<const std::string*> values;
		for (; ei != ee; ++ei)
		{
			ranker.insert( ei->second / m_data->norm, values.size());
			values.push_back( &ee->first);
		}
		std::vector<Ranker::Element> result = ranker.result();
		std::vector<Ranker::Element>::const_iterator ri = result.begin(), re = result.end();
		for (; ri != re; ++ri)
		{
			rt.push_back( SummaryElement( m_data->resultname, *values[ ri->idx], ri->weight));
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summary: %s"), METHOD_NAME, *m_errorhnd, std::vector<SummaryElement>());
}


void SummarizerFunctionInstanceAccumulateNear::addStringParameter( const std::string& name, const std::string& value)
{
	try
	{
		if (utils::caseInsensitiveEquals( name, "match") || utils::caseInsensitiveEquals( name, "struct"))
		{
			m_errorhnd->report( _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as string"), name.c_str(), METHOD_NAME);
		}
		else if (utils::caseInsensitiveEquals( name, "type"))
		{
			m_data->type = value;
			if (m_data->resultname.empty())
			{
				m_data->resultname = value;
			}
		}
		else if (utils::caseInsensitiveEquals( name, "result"))
		{
			m_data->resultname = value;
		}
		else if (utils::caseInsensitiveEquals( name, "cardinality") && !value.empty() && value[value.size()-1] == '%')
		{
			m_data->cardinality = 0;
			m_data->cardinality_frac = utils::tofraction( value);
		}
		else if (utils::caseInsensitiveEquals( name, "cofactor")
			|| utils::caseInsensitiveEquals( name, "norm")
			|| utils::caseInsensitiveEquals( name, "nofranks")
			|| utils::caseInsensitiveEquals( name, "range"))
		{
			m_errorhnd->report( _TXT("parameter '%s' for summarizer '%s' expected to be defined as string and not as numeric value"), name.c_str(), METHOD_NAME);
		}
		else
		{
			throw strus::runtime_error( _TXT("unknown '%s' summarization function parameter '%s'"), METHOD_NAME, name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding string parameter to '%s' summarizer: %s"), METHOD_NAME, *m_errorhnd);
}

void SummarizerFunctionInstanceAccumulateNear::addNumericParameter( const std::string& name, const NumericVariant& value)
{
	if (utils::caseInsensitiveEquals( name, "match") || utils::caseInsensitiveEquals( name, "struct"))
	{
		m_errorhnd->report( _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as numeric value"), name.c_str(), METHOD_NAME);
	}
	else if (utils::caseInsensitiveEquals( name, "type")
		|| utils::caseInsensitiveEquals( name, "result"))
	{
		m_errorhnd->report( _TXT("no numeric value expected for parameter '%s' in summarization function '%s'"), name.c_str(), METHOD_NAME);
	}
	else if (utils::caseInsensitiveEquals( name, "cofactor"))
	{
		m_data->cofactor = value.tofloat();
	}
	else if (utils::caseInsensitiveEquals( name, "norm"))
	{
		m_data->cofactor = value.tofloat();
	}
	else if (utils::caseInsensitiveEquals( name, "range"))
	{
		m_data->range = value.touint();
	}
	else if (utils::caseInsensitiveEquals( name, "nofranks"))
	{
		m_data->nofranks = value.touint();
	}
	else if (utils::caseInsensitiveEquals( name, "cardinality"))
	{
		m_data->cardinality = value.touint();
		m_data->cardinality_frac = 0.0;
	}
	else
	{
		m_errorhnd->report( _TXT("unknown '%s' summarization function parameter '%s'"), METHOD_NAME, name.c_str());
	}
}

void SummarizerFunctionInstanceAccumulateNear::defineResultName(
		const std::string& resultname,
		const std::string& itemname)
{
	try
	{
		throw strus::runtime_error(_TXT("no result rename defined for '%s' summarizer"), METHOD_NAME);
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error defining result name of '%s' summarizer: %s"), METHOD_NAME, *m_errorhnd);
}

SummarizerFunctionContextInterface* SummarizerFunctionInstanceAccumulateNear::createFunctionContext(
		const StorageClientInterface* storage,
		MetaDataReaderInterface*,
		const GlobalStatistics& stats) const
{
	if (m_data->type.empty())
	{
		m_errorhnd->report( _TXT( "empty forward index type definition (parameter 'type') in match phrase summarizer configuration"));
	}
	try
	{
		double nofCollectionDocuments = stats.nofDocumentsInserted()>=0?stats.nofDocumentsInserted():(GlobalCounter)storage->nofDocumentsInserted();
		return new SummarizerFunctionContextAccumulateNear( storage, m_processor, m_data, nofCollectionDocuments, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' summarizer: %s"), METHOD_NAME, *m_errorhnd, 0);
}

std::string SummarizerFunctionInstanceAccumulateNear::tostring() const
{
	try
	{
		std::ostringstream rt;
		rt << "type='" << m_data->type << "'";
		rt << ", result='" << m_data->resultname << "'";
		rt << ", cofactor=" << m_data->cofactor;
		rt << ", nofranks=" << m_data->nofranks;
		rt << ", cardinality=" << m_data->cardinality;
		rt << ", range=" << m_data->range;
		return rt.str();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error mapping '%s' summarizer to string: %s"), METHOD_NAME, *m_errorhnd, std::string());
}


SummarizerFunctionInstanceInterface* SummarizerFunctionAccumulateNear::createInstance(
		const QueryProcessorInterface* processor) const
{
	try
	{
		return new SummarizerFunctionInstanceAccumulateNear( processor, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of '%s' summarizer: %s"), METHOD_NAME, *m_errorhnd, 0);
}



FunctionDescription SummarizerFunctionAccumulateNear::getDescription() const
{
	try
	{
		typedef FunctionDescription::Parameter P;
		FunctionDescription rt( _TXT("Extract and weight all elements in the forward index of a given type that are within a window with features specified."));
		rt( P::Feature, "match", _TXT( "defines the query features to inspect for near matches"), "");
		rt( P::Feature, "struct", _TXT( "defines a structural delimiter for interaction of features on the same result"), "");
		rt( P::String, "type", _TXT( "the forward index feature type for the content to extract"), "");
		rt( P::String, "result", _TXT( "the name of the result if not equal to type"), "");
		rt( P::String, "cofactor", _TXT( "multiplication factor for features pointing to the same result"), "");
		rt( P::String, "norm", _TXT( "normalization factor for end result weights"), "");
		rt( P::String, "nofranks", _TXT( "maximum number of ranks per document"), "");
		rt( P::String, "cardinality", _TXT( "mimimum number of features per weighted item"), "");
		rt( P::String, "range", _TXT( "maximum distance (ordinal position) of the weighted features (window size)"), "");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating summarizer function description for '%s': %s"), METHOD_NAME, *m_errorhnd, FunctionDescription());
}

