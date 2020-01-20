/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "weightingBM25pff.hpp"
#include "postingIteratorHelpers.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/functionDescription.hpp"
#include "strus/constants.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/string_conv.hpp"
#include "strus/base/numstring.hpp"
#include "strus/base/math.hpp"
#include "viewUtils.hpp"
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace strus;
#define THIS_METHOD_NAME const_cast<char*>("BM25pff")
#undef STRUS_LOWLEVEL_DEBUG

WeightingFunctionContextBM25pff::WeightingFunctionContextBM25pff(
		const StorageClientInterface* storage,
		const WeightingFunctionParameterBM25pff& parameter_,
		double nofCollectionDocuments_,
		const std::string& metadata_doclen_,
		ErrorBufferInterface* errorhnd_)
	:m_parameter(parameter_)
	,m_nofCollectionDocuments(nofCollectionDocuments_)
	,m_cardinality(parameter_.cardinality)
	,m_itrarsize(0)
	,m_structarsize(0)
	,m_paraarsize(0)
	,m_nof_maxdf_features(0)
	,m_initialized(false)
	,m_metadata(storage->createMetaDataReader())
	,m_metadata_doclen(-1)
	,m_titleitr(0)
	,m_errorhnd(errorhnd_)
{
	if (!m_metadata.get())
	{
		throw strus::runtime_error( _TXT("failed to create meta data reader: %s"), m_errorhnd->fetchError());
	}
	m_metadata_doclen = m_metadata->elementHandle( metadata_doclen_);
	if (m_metadata_doclen<0)
	{
		throw strus::runtime_error( _TXT("no meta data element '%s' for the document lenght defined"), metadata_doclen_.c_str());
	}
}

void WeightingFunctionContextBM25pff::setVariableValue( const std::string&, double)
{
	m_errorhnd->report( ErrorCodeNotImplemented, _TXT("no variables known for function '%s'"), THIS_METHOD_NAME);
}

void WeightingFunctionContextBM25pff::addWeightingFeature(
		const std::string& name_,
		PostingIteratorInterface* itr,
		double weight,
		const TermStatistics& termstats)
{
	try
	{
		if (strus::caseInsensitiveEquals( name_, "title"))
		{
			if (m_titleitr) throw std::runtime_error( _TXT("title field specified twice"));
			m_titleitr = itr;
		}
		else if (strus::caseInsensitiveEquals( name_, "struct"))
		{
			if (m_structarsize + m_structarsize > MaxNofArguments) throw std::runtime_error( _TXT("number of structure features out of range"));
			m_structar[ m_structarsize + m_paraarsize] = m_structar[ m_structarsize];
			m_structar[ m_structarsize++] = itr;
		}
		else if (strus::caseInsensitiveEquals( name_, "para"))
		{
			if (m_paraarsize + m_structarsize > MaxNofArguments) throw std::runtime_error( _TXT("number of structure features out of range"));
			m_structar[ m_structarsize + m_paraarsize] = itr;
			m_paraarsize++;
		}
		else if (strus::caseInsensitiveEquals( name_, "match"))
		{
			if (m_itrarsize > MaxNofArguments) throw std::runtime_error( _TXT("number of weighting features out of range"));

			double df = termstats.documentFrequency()>=0?termstats.documentFrequency():(GlobalCounter)itr->documentFrequency();
			double idf = strus::Math::log10( (m_nofCollectionDocuments - df + 0.5) / (df + 0.5));
			if (m_parameter.maxdf * m_nofCollectionDocuments < df)
			{
				m_relevantfeat[ m_itrarsize] = false;
				++m_nof_maxdf_features;
			}
			else
			{
				m_relevantfeat[ m_itrarsize] = true;
			}
			if (idf < 0.00001)
			{
				idf = 0.00001;
			}
			m_itrar[ m_itrarsize++] = itr;
			m_idfar.push( idf * weight);
		}
		else
		{
			m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' weighting function feature parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding weighting feature to '%s' weighting: %s"), THIS_METHOD_NAME, *m_errorhnd);
}

void WeightingFunctionContextBM25pff::calcTitleFfIncrements(
		WeightingData& wdata,
		ProximityWeightAccumulator::WeightArray& result)
{
	unsigned long poset = 0;
	Index nofFeaturesInTitle = 0;
	for (std::size_t ii=0; ii<m_itrarsize; ++ii)
	{
		if (!wdata.valid_itrar[ii]) continue;

		Index pos = wdata.valid_itrar[ii]->skipPos( wdata.titlestart);
		if (pos && pos < wdata.titleend)
		{
			Index endpos = pos + wdata.valid_itrar[ii]->length();
			for (; pos < endpos; ++pos)
			{
				unsigned int posetidx = (pos-wdata.titlestart);
				if ((poset & (1<<posetidx)) == 0)
				{
					poset |= (1<<posetidx);
					++nofFeaturesInTitle;
				}
			}
		}
	}
	if (wdata.titleend - wdata.titlestart < nofFeaturesInTitle)
	{
		nofFeaturesInTitle = wdata.titleend - wdata.titlestart;
	}
	double nf = 1.0 / (double)(wdata.titleend - wdata.titlestart - nofFeaturesInTitle + 1);

	for (std::size_t ii=0; ii<m_itrarsize; ++ii)
	{
		if (!wdata.valid_itrar[ii]) continue;

		Index pos = wdata.valid_itrar[ii]->skipPos( wdata.titlestart);
		if (pos && pos < wdata.titleend)
		{
			result[ ii] += m_parameter.titleinc * m_weightincr[ii] * nf;
		}
	}
}


void WeightingFunctionContextBM25pff::calcTitleWeights( WeightingData& wdata, strus::Index docno, ProximityWeightAccumulator::WeightArray& weightar)
{
	if (m_parameter.titleinc > std::numeric_limits<double>::epsilon()
		&& m_titleitr && m_titleitr->skipDoc( docno) == docno)
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

			// Calculate ff title increment weights:
			calcTitleFfIncrements( wdata, weightar);
		}
		else
		{
			wdata.titlestart = 1;
		}
	}
}

void WeightingFunctionContextBM25pff::calcWindowWeight(
		WeightingData& wdata, const PositionWindow& poswin,
		const std::pair<Index,Index>& structframe,
		const std::pair<Index,Index>& paraframe,
		ProximityWeightAccumulator::WeightArray& result)
{
	const std::size_t* window = poswin.window();
	std::size_t windowsize = poswin.size();
	Index windowpos = poswin.pos();

	// Calculate the ff increment for the current window and add it to the result:
	ProximityWeightAccumulator::weight_same_sentence(
		result, m_parameter.weight_same_sentence, m_weightincr, window, windowsize,
		wdata.valid_itrar, m_itrarsize,
		structframe);
	ProximityWeightAccumulator::weight_invdist(
		result, m_parameter.weight_invdist, m_weightincr, window, windowsize,
		wdata.valid_itrar, m_itrarsize);

	if (windowpos < 1000)
	{
		// Weight distance to start of document:
		ProximityWeightAccumulator::weight_invpos(
			result, m_parameter.weight_invpos_start, m_weightincr, 1,
			window, windowsize, wdata.valid_itrar, m_itrarsize);
	}
	if (paraframe.first)
	{
		// Weight inv distance to paragraph start:
		ProximityWeightAccumulator::weight_invpos(
			result, m_parameter.weight_invpos_para, m_weightincr, paraframe.first,
			window, windowsize, wdata.valid_itrar, m_itrarsize);
	}
	if (structframe.first)
	{
		// Weight inv distance to paragraph start:
		ProximityWeightAccumulator::weight_invpos(
			result, m_parameter.weight_invpos_struct, m_weightincr, structframe.first,
			window, windowsize, wdata.valid_itrar, m_itrarsize);
	}
}

void WeightingFunctionContextBM25pff::calcProximityFfIncrements(
		WeightingData& wdata,
		ProximityWeightAccumulator::WeightArray& result)
{
	Index lastEndPos = 0;
	unsigned int lastElementCnt = 0;
	PositionWindow poswin( wdata.valid_itrar, m_itrarsize, m_parameter.windowsize, m_cardinality,
				1U/*firstpos*/, PositionWindow::MaxWin);
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

		// Check if window is overlapping a paragraph. In this case to not use it for ff increments:
		std::pair<Index,Index> paraframe = wdata.paraiter.skipPos( windowpos);
		if (paraframe.first && paraframe.second < windowpos + windowspan) continue;
	
		// Calculate sentence frame:
		std::pair<Index,Index> structframe = wdata.structiter.skipPos( windowpos);

		// Calculate ff increments of this window:
		calcWindowWeight( wdata, poswin, structframe, paraframe, result);
	}
}

void WeightingFunctionContextBM25pff::logCalcProximityFfIncrements(
		std::ostream& out,
		WeightingData& wdata,
		ProximityWeightAccumulator::WeightArray& result)
{
	Index lastEndPos = 0;
	unsigned int lastElementCnt = 0;
	PositionWindow poswin( wdata.valid_itrar, m_itrarsize, m_parameter.windowsize, m_cardinality,
				1U/*firstpos*/, PositionWindow::MaxWin);
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

		// Check if window is overlapping a paragraph. In this case to not use it for ff increments:
		std::pair<Index,Index> paraframe = wdata.paraiter.skipPos( windowpos);
		if (paraframe.first && paraframe.second < windowpos + windowspan) continue;
	
		// Calculate sentence frame:
		std::pair<Index,Index> structframe = wdata.structiter.skipPos( windowpos);

		// Calculate ff increments of this window:
		ProximityWeightAccumulator::WeightArray row_result( result.arsize, 0.0);
		calcWindowWeight( wdata, poswin, structframe, paraframe, row_result);

		std::string result_str = row_result.tostring();
		out << string_format( _TXT( "window pos=%u, span=%u, proximity ff increments: %s"),
					windowpos, windowspan, result_str.c_str()) << std::endl;
		result.add( row_result);
	}
}

void WeightingFunctionContextBM25pff::initializeContext()
{
	if (m_cardinality == 0)
	{
		if (m_parameter.cardinality_frac > std::numeric_limits<double>::epsilon())
		{
			m_cardinality = std::max( 1U, (unsigned int)(m_itrarsize * m_parameter.cardinality_frac + 0.5));
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
	// initialize proportional ff increment weights
	m_weightincr.init( m_itrarsize);
	ProximityWeightAccumulator::proportionalAssignment( m_weightincr, 1.0, m_parameter.prop_weight_const, m_idfar);

	m_initialized = true;
}

void WeightingFunctionContextBM25pff::initWeightingData( WeightingData& wdata, strus::Index docno)
{
	callSkipDoc( docno, m_itrar, m_itrarsize, wdata.valid_itrar);
	callSkipDoc( docno, m_structar, m_structarsize + m_paraarsize, wdata.valid_structar);
	m_metadata->skipDoc( docno);
	wdata.doclen = m_metadata->getValue( m_metadata_doclen);
}

double WeightingFunctionContextBM25pff::featureWeight( const WeightingData& wdata, strus::Index docno, double idf, double weight_ff) const
{
	if (m_parameter.b)
	{
		double rel_doclen = (wdata.doclen+1) / m_parameter.avgDocLength;
		double ww = idf
				* (weight_ff * (m_parameter.k1 + 1.0))
				/ (weight_ff + m_parameter.k1
					* (1.0 - m_parameter.b + m_parameter.b * rel_doclen));
		return ww;
	}
	else
	{
		double ww = idf
				* (weight_ff * (m_parameter.k1 + 1.0))
				/ (weight_ff + m_parameter.k1 * 1.0);
		return ww;
	}
	return 0.0;
}

double WeightingFunctionContextBM25pff::call( const Index& docno)
{
	try
	{
		if (m_itrarsize == 0) return 0.0;
		if (!m_initialized) initializeContext();

		// Init data:
		WeightingData wdata( m_itrarsize, m_structarsize, m_paraarsize, m_parameter.sentencesize, m_parameter.paragraphsize);
		initWeightingData( wdata, docno);

		// Calculate artificial ff for all features:
		calcTitleWeights( wdata, docno, wdata.ffincrar);

		std::size_t featcount = 0;
		double rt = 0.0;
		std::size_t fi = 0;
		for ( ;fi != m_itrarsize; ++fi)
		{
			if (wdata.valid_itrar[ fi] && m_relevantfeat[ fi]) ++featcount;
		}
		if (featcount <= 1)
		{
			//.... Fallback to BM25 plus some title increments
			for (fi = 0; fi != m_itrarsize; ++fi)
			{
				if (!wdata.valid_itrar[ fi]) continue;
				rt += featureWeight( wdata, docno, m_idfar[ fi], wdata.valid_itrar[ fi]->frequency() + wdata.ffincrar[ fi]);
			}
		}
		else
		{
			if (m_cardinality <= m_itrarsize)
			{
				calcProximityFfIncrements( wdata, wdata.ffincrar);
			}
	
			// Calculate the BM25 weight with the artificial ff:
			for (fi = 0;fi != m_itrarsize; ++fi)
			{
				if (!wdata.valid_itrar[ fi]) continue;
	
				double sing_ff = wdata.valid_itrar[ fi]->frequency();
				if (sing_ff <= std::numeric_limits<double>::epsilon()) continue;
	
				double prox_ff = wdata.ffincrar[ fi];
				double accu_ff = m_parameter.ffbase * sing_ff + (1.0-m_parameter.ffbase) * prox_ff;
	
				rt += featureWeight( wdata, docno, m_idfar[ fi], accu_ff);
			}
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error calling weighting function '%s': %s"), THIS_METHOD_NAME, *m_errorhnd, 0.0);
}

std::string WeightingFunctionContextBM25pff::debugCall( const Index& docno)
{
	if (m_itrarsize == 0) return std::string();

	std::ostringstream out;
	out << std::fixed << std::setprecision(8);
	out << string_format( _TXT( "calculate %s"), THIS_METHOD_NAME) << std::endl;

	if (!m_initialized) initializeContext();

	// Init data:
	WeightingData wdata( m_itrarsize, m_structarsize, m_paraarsize, m_parameter.sentencesize, m_parameter.paragraphsize);
	initWeightingData( wdata, docno);

	std::string wstr = m_weightincr.tostring();
	out << string_format( _TXT("feature weights: %s, sum=%f"), wstr.c_str(), m_weightincr.sum()) << std::endl;

	// Calculate proximity ff for all features:
	WeightArray titleweightar( wdata.ffincrar.arsize, 0.0);
	calcTitleWeights( wdata, docno, titleweightar);
	std::string titleweightar_str = titleweightar.tostring();
	out << string_format( _TXT("title ff increments=%s"), titleweightar_str.c_str()) << std::endl;

	std::size_t featcount = 0;
	double res = 0.0;
	for (std::size_t fi = 0; fi != m_itrarsize; ++fi)
	{
		if (wdata.valid_itrar[ fi] && m_relevantfeat[ fi]) ++featcount;
	}
	if (featcount <= 1)
	{
		out << _TXT( "fallback to BM25 (too few query features)") << std::endl;
		for (std::size_t fi = 0; fi != m_itrarsize; ++fi)
		{
			if (!wdata.valid_itrar[ fi]) continue;
			double ff = wdata.valid_itrar[ fi]->frequency();
			double ww = featureWeight( wdata, docno, m_idfar[ fi], ff + wdata.ffincrar[ fi]);
			res += ww;
			out << string_format( _TXT("[%u] result=%f, ff=%f, title ff incr=%f, idf=%f, doclen=%u"),
						(unsigned int)fi, ww,
						ff, wdata.ffincrar[ fi],
						m_idfar[ fi], (unsigned int)wdata.doclen) << std::endl;
		}
	}
	else
	{
		if (m_cardinality <= m_itrarsize)
		{
			logCalcProximityFfIncrements( out, wdata, wdata.ffincrar);
		}
		std::string ffincrar_str = wdata.ffincrar.tostring();
		out << string_format( _TXT("proximity ff increments: %s"), ffincrar_str.c_str()) << std::endl;
	
		wdata.ffincrar.add( titleweightar);
	
		ffincrar_str = wdata.ffincrar.tostring();
		out << string_format( _TXT("accumulated ff increments: %s"),
					ffincrar_str.c_str()) << std::endl;
	
		// Calculate the BM25 weight with the proximity ff:
		std::size_t fi = 0;
		for ( ;fi != m_itrarsize; ++fi)
		{
			if (!wdata.valid_itrar[ fi]) continue;
	
			double sing_ff = wdata.valid_itrar[ fi]->frequency();
			if (sing_ff <= std::numeric_limits<double>::epsilon()) continue;
	
			double prox_ff = wdata.ffincrar[ fi];
			double accu_ff = m_parameter.ffbase * sing_ff + (1.0-m_parameter.ffbase) * prox_ff;
	
			double ww = featureWeight( wdata, docno, m_idfar[ fi], accu_ff);
			res += ww;
			out << string_format( _TXT("[%u] result=%f, accu_ff=%f, sing_ff=%f, prox_ff=%f, idf=%f, doclen=%u"),
						(unsigned int)fi, ww,
						accu_ff, sing_ff, prox_ff,
						m_idfar[ fi], (unsigned int)wdata.doclen) << std::endl;
		}
	}
	out << string_format( _TXT("sum result=%f"), res) << std::endl;
	return out.str();
}

static NumericVariant parameterValue( const std::string& name_, const std::string& value)
{
	NumericVariant rt;
	if (!rt.initFromString(value.c_str())) throw strus::runtime_error(_TXT("numeric value expected as parameter '%s' (%s)"), name_.c_str(), value.c_str());
	return rt;
}

void WeightingFunctionInstanceBM25pff::setMaxNofWeightedFields( int N)
{
	try
	{
		if (N <= 0) throw std::runtime_error( _TXT("positive number expected as argument"));
		m_parameter.maxNofWeightingFields = N;
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error '%s' weighting function set max nof weighted fields: %s"), THIS_METHOD_NAME, *m_errorhnd);
}

void WeightingFunctionInstanceBM25pff::addStringParameter( const std::string& name_, const std::string& value)
{
	try
	{
		if (strus::caseInsensitiveEquals( name_, "match") || strus::caseInsensitiveEquals( name_, "struct") || strus::caseInsensitiveEquals( name_, "para"))
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for weighting scheme '%s' expected to be defined as feature and not as string or numeric value"), name_.c_str(), THIS_METHOD_NAME);
		}
		else if (strus::caseInsensitiveEquals( name_, "metadata_doclen"))
		{
			if (value.empty()) m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("empty value passed as '%s' weighting function parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
			m_metadata_doclen = value;
		}
		else if (strus::caseInsensitiveEquals( name_, "cardinality") && !value.empty() && value[value.size()-1] == '%')
		{
			m_parameter.cardinality = 0;
			m_parameter.cardinality_frac = numstring_conv::todouble( value);
		}
		else if (strus::caseInsensitiveEquals( name_, "k1")
		||  strus::caseInsensitiveEquals( name_, "b")
		||  strus::caseInsensitiveEquals( name_, "avgdoclen")
		||  strus::caseInsensitiveEquals( name_, "titleinc")
		||  strus::caseInsensitiveEquals( name_, "cprop")
		||  strus::caseInsensitiveEquals( name_, "paragraphsize")
		||  strus::caseInsensitiveEquals( name_, "sentencesize")
		||  strus::caseInsensitiveEquals( name_, "windowsize")
		||  strus::caseInsensitiveEquals( name_, "cardinality")
		||  strus::caseInsensitiveEquals( name_, "ffbase"))
		{
			addNumericParameter( name_, parameterValue( name_, value));
		}
		else
		{
			m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' weighting function parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error '%s' weighting function add string parameter: %s"), THIS_METHOD_NAME, *m_errorhnd);
}

void WeightingFunctionInstanceBM25pff::addNumericParameter( const std::string& name_, const NumericVariant& value)
{
	if (strus::caseInsensitiveEquals( name_, "match") || strus::caseInsensitiveEquals( name_, "struct") || strus::caseInsensitiveEquals( name_, "para"))
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for weighting scheme '%s' expected to be defined as feature and not as string or numeric value"), name_.c_str(), THIS_METHOD_NAME);
	}
	else if (strus::caseInsensitiveEquals( name_, "k1"))
	{
		m_parameter.k1 = (double)value;
	}
	else if (strus::caseInsensitiveEquals( name_, "b"))
	{
		m_parameter.b = (double)value;
	}
	else if (strus::caseInsensitiveEquals( name_, "avgdoclen"))
	{
		m_parameter.avgDocLength = (double)value;
	}
	else if (strus::caseInsensitiveEquals( name_, "paragraphsize"))
	{
		m_parameter.paragraphsize = value.touint();
	}
	else if (strus::caseInsensitiveEquals( name_, "sentencesize"))
	{
		m_parameter.sentencesize = value.touint();
	}
	else if (strus::caseInsensitiveEquals( name_, "windowsize"))
	{
		m_parameter.windowsize = value.touint();
	}
	else if (strus::caseInsensitiveEquals( name_, "cardinality"))
	{
		if (value.type == NumericVariant::Int && value.toint() >= 0)
		{
			m_parameter.cardinality = value.touint();
			m_parameter.cardinality_frac = 0.0;
		}
		else if (value.type == NumericVariant::UInt)
		{
			m_parameter.cardinality = value.touint();
			m_parameter.cardinality_frac = 0.0;
		}
		else
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for weighting scheme '%s' expected to a non negative integer value"), name_.c_str(), THIS_METHOD_NAME);
		}
	}
	else if (strus::caseInsensitiveEquals( name_, "ffbase"))
	{
		m_parameter.ffbase = (double)value;
		if (m_parameter.ffbase < 0.0 || m_parameter.ffbase > 1.0)
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for weighting scheme '%s' expected to a positive floating point number between 0.0 and 1.0"), name_.c_str(), THIS_METHOD_NAME);
		}
	}
	else if (strus::caseInsensitiveEquals( name_, "maxdf"))
	{
		m_parameter.maxdf = (double)value;
		if (m_parameter.maxdf < 0.0 || m_parameter.maxdf > 1.0)
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for weighting scheme '%s' expected to a positive floating point number between 0.0 and 1.0"), name_.c_str(), THIS_METHOD_NAME);
		}
	}
	else if (strus::caseInsensitiveEquals( name_, "titleinc"))
	{
		m_parameter.titleinc = (double)value;
		if (m_parameter.titleinc < 0.0)
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for weighting scheme '%s' expected to a positive floating point number"), name_.c_str(), THIS_METHOD_NAME);
		}
	}
	else if (strus::caseInsensitiveEquals( name_, "cprop"))
	{
		m_parameter.prop_weight_const = (double)value;
		if (m_parameter.prop_weight_const < 0.0 || m_parameter.prop_weight_const > 1.0)
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for weighting scheme '%s' expected to be a floating point number between 0 and 1"), name_.c_str(), THIS_METHOD_NAME);
		}
	}
	else if (strus::caseInsensitiveEquals( name_, "metadata_doclen"))
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for weighting scheme '%s' expected to be defined as string and not as numeric value"), name_.c_str(), THIS_METHOD_NAME);
	}
	else
	{
		m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' weighting function parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
	}
}


WeightingFunctionContextInterface* WeightingFunctionInstanceBM25pff::createFunctionContext(
		const StorageClientInterface* storage_,
		const GlobalStatistics& stats) const
{
	try
	{
		GlobalCounter nofdocs = stats.nofDocumentsInserted()>=0?stats.nofDocumentsInserted():(GlobalCounter)storage_->nofDocumentsInserted();
		return new WeightingFunctionContextBM25pff(
				storage_, m_parameter, nofdocs, m_metadata_doclen, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' weighting function: %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}

StructView WeightingFunctionInstanceBM25pff::view() const
{
	try
	{
		StructView rt;
		rt( "b", m_parameter.b);
		rt( "k1", m_parameter.k1);
		rt( "avgdoclen", m_parameter.avgDocLength);
		rt( "metadata_doclen", m_metadata_doclen);
		
		rt( "paragraphsize", m_parameter.paragraphsize);
		rt( "sentencesize", m_parameter.sentencesize);
		rt( "windowsize", m_parameter.windowsize);
		if (m_parameter.cardinality_frac > std::numeric_limits<float>::epsilon())
		{
			rt( "cardinality", strus::string_format( "%u%%", (unsigned int)(m_parameter.cardinality_frac * 100 + 0.5)));
		}
		else
		{
			rt( "cardinality", m_parameter.cardinality);
		}
		rt( "ffbase", m_parameter.ffbase);
		rt( "maxdf", m_parameter.maxdf);
		rt( "titleinc", m_parameter.titleinc);
		rt( "cprop", m_parameter.prop_weight_const);
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summarizer introspection view: %s"), THIS_METHOD_NAME, *m_errorhnd, std::string());
}

WeightingFunctionInstanceInterface* WeightingFunctionBM25pff::createInstance(
		const QueryProcessorInterface* ) const
{
	try
	{
		return new WeightingFunctionInstanceBM25pff( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' function instance: %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}

StructView WeightingFunctionBM25pff::view() const
{
	try
	{
		typedef FunctionDescription P;
		FunctionDescription rt( name(), _TXT("Calculate the document weight with the weighting scheme \"BM25pff\". This is \"BM25\" where the feature frequency is counted by 1.0 per feature only for features with the maximum proximity score. The proximity score is a measure that takes the proximity of other query features into account"));
		rt( P::Feature, "match", _TXT( "defines the query features to weight"), "");
		rt( P::Feature, "struct", _TXT( "defines the delimiter for structures"), "");
		rt( P::Feature, "para", _TXT( "defines the delimiter for paragraphs (windows used for proximity weighting must not overlap paragraph borders)"), "");
		rt( P::Feature, "title", _TXT( "defines the title field (used for weighting increment of features appearing in title)"), "");
		rt( P::Numeric, "k1", _TXT("parameter of the BM25pff weighting scheme"), "1:1000");
		rt( P::Numeric, "b", _TXT("parameter of the BM25pff weighting scheme"), "0.0001:1000");
		rt( P::Numeric, "titleinc", _TXT("ff increment for title features"), "0.0:");
		rt( P::Numeric, "cprop", _TXT("constant part of idf proportional feature weight"), "0.0:1.0");
		rt( P::Numeric, "paragraphsize", _TXT("the estimated size of a paragraph"), "");
		rt( P::Numeric, "sentencesize", _TXT("the estimated size of a sentence"), "");
		rt( P::Numeric, "windowsize", _TXT("the size of the window used for finding features to increment proximity scores"), "");
		rt( P::Numeric, "cardinality", _TXT("the number of query features a proximity score window must contain to be considered (optional, default is all features, percentage of input features specified with '%' suffix)"), "");
		rt( P::Numeric, "ffbase", _TXT( "value in the range from 0.0 to 1.0 specifying the percentage of the constant score on the proximity ff for every feature occurrence. (with 1.0 the scheme is plain BM25)"), "0.0:1.0");
		rt( P::Numeric, "avgdoclen", _TXT("the average document lenght"), "0:");
		rt( P::Numeric, "maxdf", _TXT("the maximum df as fraction of the collection size"), "0:");
		rt( P::Metadata, "metadata_doclen", _TXT("the meta data element name referencing the document lenght for each document weighted"), "");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating weighting function description for '%s': %s"), THIS_METHOD_NAME, *m_errorhnd, FunctionDescription());
}

