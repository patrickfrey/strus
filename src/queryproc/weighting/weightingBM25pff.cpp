/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "weightingBM25pff.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "strus/constants.hpp"
#include <cmath>
#include <ctime>

using namespace strus;
#define WEIGHTING_SCHEME_NAME "BM25pff"
#undef STRUS_LOWLEVEL_DEBUG

WeightingFunctionContextBM25pff::WeightingFunctionContextBM25pff(
		const StorageClientInterface* storage,
		MetaDataReaderInterface* metadata_,
		const WeightingFunctionParameterBM25pff& parameter_,
		double nofCollectionDocuments_,
		const std::string& metadata_doclen_,
		ErrorBufferInterface* errorhnd_)
	:m_parameter(parameter_)
	,m_nofCollectionDocuments(nofCollectionDocuments_)
	,m_itrarsize(0)
	,m_structarsize(0)
	,m_paraarsize(0)
	,m_nof_maxdf_features(0)
	,m_initialized(false)
	,m_metadata(metadata_)
	,m_metadata_doclen(metadata_->elementHandle( metadata_doclen_.empty()?std::string("doclen"):metadata_doclen_))
	,m_titleitr(0)
	,m_errorhnd(errorhnd_)
{
	if (m_metadata_doclen<0)
	{
		throw strus::runtime_error( _TXT("no meta data element for the document lenght defined"));
	}
}

void WeightingFunctionContextBM25pff::addWeightingFeature(
		const std::string& name,
		PostingIteratorInterface* itr,
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
			double idf = std::log10( (m_nofCollectionDocuments - df + 0.5) / (df + 0.5));
			if (m_parameter.maxdf * m_nofCollectionDocuments < df)
			{
				m_maxdist_featar[ m_itrarsize] = (m_parameter.windowsize > 5)?5:m_parameter.windowsize;
				++m_nof_maxdf_features;
			}
			else
			{
				m_maxdist_featar[ m_itrarsize] = m_parameter.windowsize;
			}
			if (idf < 0.00001)
			{
				idf = 0.00001;
			}
			m_itrar[ m_itrarsize++] = itr;
			m_idfar.add( idf * weight);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "add feature idf=" << idf << " weight=" << weight << std::endl;
#endif
		}
		else
		{
			m_errorhnd->report( _TXT("unknown '%s' weighting function feature parameter '%s'"), WEIGHTING_SCHEME_NAME, name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding weighting feature to '%s' weighting: %s"), WEIGHTING_SCHEME_NAME, *m_errorhnd);
}


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

static void calcTitleFfIncrements(
		ProximityWeightAccumulator::WeightArray& result,
		const Index& titlestart,
		const Index& titleend,
		double titleweight,
		const ProximityWeightAccumulator::WeightArray& weightincr,
		PostingIteratorInterface** itrar, std::size_t itrarsize)
{
	unsigned long poset = 0;
	Index nofFeaturesInTitle = 0;
	for (std::size_t ii=0; ii<itrarsize; ++ii)
	{
		Index pos = itrar[ii]->skipPos(0);
		while (pos && pos < titleend && pos >= titlestart)
		{
			if ((poset & (1<<pos)) == 0)
			{
				poset |= (1<<pos);
				++nofFeaturesInTitle;
				break;
			}
			pos = itrar[ii]->skipPos( pos+1);
		}
	}
	if (titleend - titlestart < nofFeaturesInTitle)
	{
		nofFeaturesInTitle = titleend - titlestart;
	}
	double nf = 1.0 / (double)(titleend - titlestart - nofFeaturesInTitle + 1);

	for (std::size_t ii=0; ii<itrarsize; ++ii)
	{
		Index pos = itrar[ii]->skipPos(0);
		if (pos && pos < titleend && pos >= titlestart)
		{
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "weighting title weight " << titleweight << " incr " << weightincr[ii] << " norm " << nf << " => " << (titleweight * weightincr[ii] * nf) << std::endl;
#endif
			result[ ii] += titleweight * weightincr[ii] * nf;
		}
	}
}

static void calcProximityFfIncrements(
			ProximityWeightAccumulator::WeightArray& result_abs,
			ProximityWeightAccumulator::WeightArray& result_rel,
			double resultincr_abs_bias,
			const Index& firstpos,
			const ProximityWeightAccumulator::WeightArray& weightincr,
			unsigned int maxwindowsize, unsigned int cardinality,
			const Index* maxdist_featar,
			const double* normfactorar,
			PostingIteratorInterface** itrar, std::size_t itrarsize,
			PostingIteratorInterface** structar, std::size_t structarsize,
			PostingIteratorInterface** paraar, std::size_t parasize,
			PostingIteratorInterface** exclar, std::size_t exclsize)
{
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
		double normfactor = normfactorar[ poswin.size()-1];

		// Check windows exclusion by features:
		std::size_t ei=0;
		for (; ei<exclsize; ++ei)
		{
			std::size_t wi=0;
			for (; wi<windowsize && itrar[ window[wi]] != exclar[ei]; ++wi) {}
			if (wi < windowsize) break;
		}
		if (ei < exclsize) continue;

		// Calculate the next paragraph element that could be overlapped by the current window:
		while (nextPara && nextPara < windowpos)
		{
			prevPara = nextPara;
			nextPara = callSkipPos( prevPara+1, paraar, parasize);
		}
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "weighting window " << windowpos << "..." << (windowpos+windowspan) << " normalization " << normfactor << " para " << prevPara << "|" << nextPara << std::endl;
#endif
		// Check if window is overlapping a paragraph. In this case to not use it for weighting:
		if (nextPara && nextPara < windowpos + windowspan) continue;

		ProximityWeightAccumulator::WeightArray result( result_abs.arsize);

		// Calculate the ff increment for the current window and add it to the result:
		ProximityWeightAccumulator::weight_same_sentence(
			result, 0.3 * normfactor, weightincr, window, windowsize, maxdist_featar, itrar, itrarsize, structar, structarsize);
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "\tff incr [same sentence] " << result.tostring() << std::endl;
#endif
		ProximityWeightAccumulator::weight_imm_follow(
			result, 0.4 * normfactor, weightincr, window, windowsize, itrar, itrarsize);
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "\tff incr [imm follow] " << result.tostring() << std::endl;
#endif
		ProximityWeightAccumulator::weight_invdist(
			result, 0.3 * normfactor, weightincr, window, windowsize, itrar, itrarsize);
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "\tff incr [inv distance] " << result.tostring() << std::endl;
#endif
		if (windowpos - firstpos < 1000)
		{
			// Weight inv distance from start of document:
			ProximityWeightAccumulator::weight_invpos(
				result, 0.5 * normfactor, weightincr, firstpos, itrar, itrarsize);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "\tff incr [inv distance begin] " << result.tostring() << std::endl;
#endif
		}
		if (prevPara && windowpos >= prevPara)
		{
			// Weight inv distance to last paragraph start:
			ProximityWeightAccumulator::weight_invpos(
				result, 0.3 * normfactor, weightincr, prevPara, itrar, itrarsize);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "\tff incr [inv distance paragraph] " << result.tostring() << std::endl;
#endif
		}
		for (size_t ri=0; ri<result.arsize; ++ri)
		{
			double winc = result.ar[ ri];
			if (winc > resultincr_abs_bias)
			{
				result_abs.ar[ ri] += winc;
			}
			else
			{
				result_rel.ar[ ri] += winc;
			}
		}
	}
}

static double normalize_0_max( double value, double maxvalue)
{
	return tanh( value/maxvalue) * maxvalue;
}

double WeightingFunctionContextBM25pff::call( const Index& docno)
{
	try
	{
		if (!m_initialized)
		{
			if (m_itrarsize == 0)
			{
				return 0.0;
			}
			if (m_itrarsize < m_parameter.cardinality || m_parameter.cardinality == 0)
			{
				m_parameter.cardinality = m_itrarsize;
			}
			if (m_nof_maxdf_features >= m_parameter.cardinality)
			{
				m_parameter.cardinality = m_nof_maxdf_features+1;
				//... at least on feature in the windows visited must fulfill the maxdf criterion
			}
			// initialize proportional ff increment weights
			m_weightincr.init( m_itrarsize);
			ProximityWeightAccumulator::proportionalAssignment( m_weightincr, 1.0, 0.3, m_idfar);
			m_initialized = true;

			for (std::size_t ii=0; ii<m_itrarsize; ++ii)
			{
				m_normfactorar[ ii] = 1.0 / sqrt(m_itrarsize - ii);
			}
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "proportional weights array: " << m_weightincr.tostring() << " sum " << m_weightincr.sum() << std::endl;
#endif
		}
		// Init document iterators:
		callSkipDoc( docno, m_itrar, m_itrarsize);
		callSkipDoc( docno, m_structar, m_structarsize);
		callSkipDoc( docno, m_paraar, m_paraarsize);
		m_metadata->skipDoc( docno);

		// Define document length:
		double doclen = m_metadata->getValue( m_metadata_doclen);

		// Define the title field and the search start position:
		Index titlestart = 1;
		Index titleend = 1;
		if (m_titleitr && m_titleitr->skipDoc( docno) == docno)
		{
			titlestart = m_titleitr->skipPos(0);
			if (titlestart)
			{
				Index ti = titleend = titlestart;
				while (0!=(ti=m_titleitr->skipPos(ti+1)))
				{
					titleend = ti;
				}
				++titleend;
			}
			else
			{
				titlestart = 1;
			}
		}
		Index firstpos = titleend;

		// Define the structure of accumulated proximity weights:
		ProximityWeightAccumulator::WeightArray ffincrar_abs( m_itrarsize);
		ProximityWeightAccumulator::WeightArray ffincrar_rel( m_itrarsize);

		// Calculate ff title increment weights:
		double tiweight = m_parameter.tidocnorm > 0 ? tanh( doclen / (double)m_parameter.tidocnorm):1.0;
		calcTitleFfIncrements(
			ffincrar_abs, titlestart, titleend, m_parameter.titleinc * tiweight, m_weightincr, m_itrar, m_itrarsize);
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "accumulated ff incr [title terms] " << ffincrar_abs.tostring() << std::endl;
#endif

		// Build array of title terms:
		PostingIteratorInterface* titleTerms[ MaxNofArguments];
		std::size_t nofTitleTerms = 0;
		double proxffbias_title_part = 0.0;
		std::size_t ti=0,te=m_itrarsize;
		for (; ti < te; ++ti)
		{
			Index pos = m_itrar[ti]->skipPos( 0);
			if (pos < firstpos)
			{
				titleTerms[ nofTitleTerms++] = m_itrar[ ti];
				proxffbias_title_part += m_weightincr[ ti];
			}
		}

		// Calculate ff proximity increment weights for all features:
		if (m_parameter.cardinality <= m_itrarsize)
		{
			calcProximityFfIncrements(
				ffincrar_abs, ffincrar_rel, m_parameter.proxffbias,
				1, m_weightincr, m_parameter.windowsize, m_parameter.cardinality,
				m_maxdist_featar, m_normfactorar,
				m_itrar, m_itrarsize, m_structar, m_structarsize, m_paraar, m_paraarsize, 0, 0);
			// Calculate ff proximity increment weights for all non title features:
			if (nofTitleTerms && m_itrarsize > nofTitleTerms)
			{
				unsigned int notitle_cardinality = m_itrarsize - nofTitleTerms;
				if (m_parameter.cardinality < notitle_cardinality)
				{
					notitle_cardinality = m_parameter.cardinality;
				}
#ifdef STRUS_LOWLEVEL_DEBUG
				std::cout << "do second pass without title terms:" << std::endl;
#endif
				double proxffbias_title = m_parameter.proxffbias - (m_parameter.proxffbias * proxffbias_title_part);
				calcProximityFfIncrements(
					ffincrar_abs, ffincrar_rel, proxffbias_title,
					firstpos, m_weightincr, m_parameter.windowsize, notitle_cardinality,
					m_maxdist_featar, m_normfactorar,
					m_itrar, m_itrarsize, m_structar, m_structarsize,
					m_paraar, m_paraarsize, titleTerms, nofTitleTerms);
			}
		}
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "final accumulated ff increments:"
			  << " absolute " << ffincrar_abs.tostring()
			  << " relative " << ffincrar_rel.tostring() << std::endl;
#endif
		double rt = 0.0;
		std::size_t fi = 0;
		for ( ;fi != m_itrarsize; ++fi)
		{
			double ff = m_itrar[ fi]->frequency();
			if (ff <= std::numeric_limits<double>::epsilon()) continue;
			double proxff_rel = ffincrar_rel[ fi];
			double proxff_abs = ffincrar_abs[ fi];

			if (m_parameter.fftie>0)
			{
				ff = normalize_0_max( ff, (double)m_parameter.fftie);
			}
			double proxff = proxff_abs;
			if (m_parameter.proxfftie>0)
			{
				proxff += normalize_0_max( proxff_rel, (double)m_parameter.proxfftie);
			}
			else
			{
				proxff += proxff_rel;
			}
			double weight_ff = m_parameter.ffbase * ff + (1.0-m_parameter.ffbase) * proxff;

#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "proximity ff [" << m_itrar[ fi]->featureid() << "] ff=" << ff << " proximity weight ff=" << weight_ff << std::endl;
#endif
			if (m_parameter.b)
			{
				double rel_doclen = (doclen+1) / m_parameter.avgDocLength;
#ifdef STRUS_LOWLEVEL_DEBUG
				double ww = m_idfar[ fi]
						* (weight_ff * (m_parameter.k1 + 1.0))
						/ (weight_ff + m_parameter.k1 
							* (1.0 - m_parameter.b + m_parameter.b * rel_doclen));
				std::cout << "idf[" << fi << "]=" << m_idfar[ fi] << " doclen=" << doclen << " weight=" << ww << std::endl;
#endif
				rt += m_idfar[ fi]
					* (weight_ff * (m_parameter.k1 + 1.0))
					/ (weight_ff + m_parameter.k1
						* (1.0 - m_parameter.b + m_parameter.b * rel_doclen));
			}
			else
			{
#ifdef STRUS_LOWLEVEL_DEBUG
				double ww = m_idfar[ fi]
						* (weight_ff * (m_parameter.k1 + 1.0))
						/ (weight_ff + m_parameter.k1 * 1.0);
				std::cout << "idf[" << fi << "]=" << m_idfar[ fi] << " weight=" << ww << std::endl;
#endif
				rt += m_idfar[ fi]
					* (weight_ff * (m_parameter.k1 + 1.0))
					/ (weight_ff + m_parameter.k1 * 1.0);
			}
		}
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "calculated weight " << rt << ", k1=" << m_parameter.k1 << ", b=" << m_parameter.b << " ,avgdoclen=" << m_parameter.avgDocLength << std::endl;
#endif
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error calling weighting function '%s': %s"), WEIGHTING_SCHEME_NAME, *m_errorhnd, 0.0);
}

static NumericVariant parameterValue( const std::string& name, const std::string& value)
{
	NumericVariant rt;
	if (!rt.initFromString(value.c_str())) throw strus::runtime_error(_TXT("numeric value expected as parameter '%s' (%s)"), name.c_str(), value.c_str());
	return rt;
}

void WeightingFunctionInstanceBM25pff::addStringParameter( const std::string& name, const std::string& value)
{
	try
	{
		if (utils::caseInsensitiveEquals( name, "match") || utils::caseInsensitiveEquals( name, "struct") || utils::caseInsensitiveEquals( name, "para"))
		{
			m_errorhnd->report( _TXT("parameter '%s' for weighting scheme '%s' expected to be defined as feature and not as string or numeric value"), name.c_str(), WEIGHTING_SCHEME_NAME);
		}
		else if (utils::caseInsensitiveEquals( name, "metadata_doclen"))
		{
			m_metadata_doclen = value;
			if (value.empty()) m_errorhnd->report( _TXT("empty value passed as '%s' weighting function parameter '%s'"), WEIGHTING_SCHEME_NAME, name.c_str());
		}
		else if (utils::caseInsensitiveEquals( name, "k1")
		||  utils::caseInsensitiveEquals( name, "b")
		||  utils::caseInsensitiveEquals( name, "avgdoclen")
		||  utils::caseInsensitiveEquals( name, "titleinc")
		||  utils::caseInsensitiveEquals( name, "tidocnorm")
		||  utils::caseInsensitiveEquals( name, "windowsize")
		||  utils::caseInsensitiveEquals( name, "cardinality")
		||  utils::caseInsensitiveEquals( name, "ffbase")
		||  utils::caseInsensitiveEquals( name, "fftie")
		||  utils::caseInsensitiveEquals( name, "proxffbias")
		||  utils::caseInsensitiveEquals( name, "proxfftie"))
		{
			addNumericParameter( name, parameterValue( name, value));
		}
		else
		{
			m_errorhnd->report( _TXT("unknown '%s' weighting function parameter '%s'"), WEIGHTING_SCHEME_NAME, name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error '%s' weighting function add string parameter: %s"), WEIGHTING_SCHEME_NAME, *m_errorhnd);
}

void WeightingFunctionInstanceBM25pff::addNumericParameter( const std::string& name, const NumericVariant& value)
{
	if (utils::caseInsensitiveEquals( name, "match") || utils::caseInsensitiveEquals( name, "struct") || utils::caseInsensitiveEquals( name, "para"))
	{
		m_errorhnd->report( _TXT("parameter '%s' for weighting scheme '%s' expected to be defined as feature and not as string or numeric value"), name.c_str(), WEIGHTING_SCHEME_NAME);
	}
	else if (utils::caseInsensitiveEquals( name, "k1"))
	{
		m_parameter.k1 = (double)value;
	}
	else if (utils::caseInsensitiveEquals( name, "b"))
	{
		m_parameter.b = (double)value;
	}
	else if (utils::caseInsensitiveEquals( name, "avgdoclen"))
	{
		m_parameter.avgDocLength = (double)value;
	}
	else if (utils::caseInsensitiveEquals( name, "windowsize"))
	{
		if (value.type == NumericVariant::Int && value.toint() > 0)
		{
			m_parameter.windowsize = value.touint();
		}
		else if (value.type == NumericVariant::UInt && value.touint() > 0)
		{
			m_parameter.windowsize = value.touint();
		}
		else
		{
			m_errorhnd->report( _TXT("parameter '%s' for weighting scheme '%s' expected to a positive integer value"), name.c_str(), WEIGHTING_SCHEME_NAME);
		}
	}
	else if (utils::caseInsensitiveEquals( name, "cardinality"))
	{
		if (value.type == NumericVariant::Int && value.toint() >= 0)
		{
			m_parameter.cardinality = value.touint();
		}
		else if (value.type == NumericVariant::UInt)
		{
			m_parameter.cardinality = value.touint();
		}
		else
		{
			m_errorhnd->report( _TXT("parameter '%s' for weighting scheme '%s' expected to a non negative integer value"), name.c_str(), WEIGHTING_SCHEME_NAME);
		}
	}
	else if (utils::caseInsensitiveEquals( name, "ffbase"))
	{
		m_parameter.ffbase = (double)value;
		if (m_parameter.ffbase < 0.0 || m_parameter.ffbase > 1.0)
		{
			m_errorhnd->report( _TXT("parameter '%s' for weighting scheme '%s' expected to a positive floating point number between 0.0 and 1.0"), name.c_str(), WEIGHTING_SCHEME_NAME);
		}
	}
	else if (utils::caseInsensitiveEquals( name, "fftie"))
	{
		if (value.type == NumericVariant::Int && value.toint() >= 0)
		{
			m_parameter.fftie = value.touint();
		}
		else if (value.type == NumericVariant::UInt)
		{
			m_parameter.fftie = value.touint();
		}
		else
		{
			m_errorhnd->report( _TXT("parameter '%s' for weighting scheme '%s' expected to a non negative integer value"), name.c_str(), WEIGHTING_SCHEME_NAME);
		}
	}
	else if (utils::caseInsensitiveEquals( name, "proxffbias"))
	{
		m_parameter.proxffbias = (double)value;
		if (m_parameter.proxffbias < 0.0 || m_parameter.proxffbias > 1.0)
		{
			m_errorhnd->report( _TXT("parameter '%s' for weighting scheme '%s' expected to a positive floating point number between 0.0 and 1.0"), name.c_str(), WEIGHTING_SCHEME_NAME);
		}
	}
	else if (utils::caseInsensitiveEquals( name, "proxfftie"))
	{
		if (value.type == NumericVariant::Int && value.toint() >= 0)
		{
			m_parameter.proxfftie = value.touint();
		}
		else if (value.type == NumericVariant::UInt)
		{
			m_parameter.proxfftie = value.touint();
		}
		else
		{
			m_errorhnd->report( _TXT("parameter '%s' for weighting scheme '%s' expected to a non negative integer value"), name.c_str(), WEIGHTING_SCHEME_NAME);
		}
	}
	else if (utils::caseInsensitiveEquals( name, "maxdf"))
	{
		m_parameter.maxdf = (double)value;
		if (m_parameter.maxdf < 0.0 || m_parameter.maxdf > 1.0)
		{
			m_errorhnd->report( _TXT("parameter '%s' for weighting scheme '%s' expected to a positive floating point number between 0.0 and 1.0"), name.c_str(), WEIGHTING_SCHEME_NAME);
		}
	}
	else if (utils::caseInsensitiveEquals( name, "titleinc"))
	{
		m_parameter.titleinc = (double)value;
		if (m_parameter.titleinc < 0.0)
		{
			m_errorhnd->report( _TXT("parameter '%s' for weighting scheme '%s' expected to a positive floating point number"), name.c_str(), WEIGHTING_SCHEME_NAME);
		}
	}
	else if (utils::caseInsensitiveEquals( name, "tidocnorm"))
	{
		if (value.type == NumericVariant::Int && value.toint() >= 0)
		{
			m_parameter.tidocnorm = value.touint();
		}
		else if (value.type == NumericVariant::UInt)
		{
			m_parameter.tidocnorm = value.touint();
		}
		else
		{
			m_errorhnd->report( _TXT("parameter '%s' for weighting scheme '%s' expected to a non negative integer value"), name.c_str(), WEIGHTING_SCHEME_NAME);
		}
	}
	else if (utils::caseInsensitiveEquals( name, "metadata_doclen"))
	{
		m_errorhnd->report( _TXT("parameter '%s' for weighting scheme '%s' expected to be defined as string and not as numeric value"), name.c_str(), WEIGHTING_SCHEME_NAME);
	}
	else
	{
		m_errorhnd->report( _TXT("unknown '%s' weighting function parameter '%s'"), WEIGHTING_SCHEME_NAME, name.c_str());
	}
}


WeightingFunctionContextInterface* WeightingFunctionInstanceBM25pff::createFunctionContext(
		const StorageClientInterface* storage_,
		MetaDataReaderInterface* metadata,
		const GlobalStatistics& stats) const
{
	try
	{
		GlobalCounter nofdocs = stats.nofDocumentsInserted()>=0?stats.nofDocumentsInserted():(GlobalCounter)storage_->nofDocumentsInserted();
		return new WeightingFunctionContextBM25pff(
				storage_, metadata, m_parameter, nofdocs, m_metadata_doclen, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' weighting function: %s"), WEIGHTING_SCHEME_NAME, *m_errorhnd, 0);
}

std::string WeightingFunctionInstanceBM25pff::tostring() const
{
	
	try
	{
		std::ostringstream rt;
		rt << std::setw(2) << std::setprecision(5)
			<< "b=" << m_parameter.b
			<< ", k1=" << m_parameter.k1
			<< ", avgdoclen=" << m_parameter.avgDocLength
			<< ", windowsize=" << m_parameter.windowsize
			<< ", cardinality=" << m_parameter.cardinality
			<< ", ffbase=" << m_parameter.ffbase
			<< ", fftie=" << m_parameter.fftie
			<< ", proxffbias=" << m_parameter.proxffbias
			<< ", proxfftie=" << m_parameter.proxfftie
			<< ", maxdf=" << m_parameter.maxdf
			<< ", titleinc=" << m_parameter.titleinc
			<< ", tidocnorm=" << m_parameter.tidocnorm
			<< ", metadata_doclen=" << m_metadata_doclen
			;
		return rt.str();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error mapping '%s' weighting function to string: %s"), WEIGHTING_SCHEME_NAME, *m_errorhnd, std::string());
}


WeightingFunctionInstanceInterface* WeightingFunctionBM25pff::createInstance(
		const QueryProcessorInterface* ) const
{
	try
	{
		return new WeightingFunctionInstanceBM25pff( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' function instance: %s"), WEIGHTING_SCHEME_NAME, *m_errorhnd, 0);
}

FunctionDescription WeightingFunctionBM25pff::getDescription() const
{
	try
	{
		typedef FunctionDescription::Parameter P;
		FunctionDescription rt(_TXT("Calculate the document weight with the weighting scheme \"BM25pff\". This is \"BM25\" where the feature frequency is counted by 1.0 per feature only for features with the maximum proximity score. The proximity score is a measure that takes the proximity of other query features into account"));
		rt( P::Feature, "match", _TXT( "defines the query features to weight"), "");
		rt( P::Feature, "struct", _TXT( "defines the delimiter for structures"), "");
		rt( P::Feature, "para", _TXT( "defines the delimiter for paragraphs (windows used for proximity weighting must not overlap paragraph borders)"), "");
		rt( P::Feature, "title", _TXT( "defines the title field (used for weighting increment of features appearing in title)"), "");
		rt( P::Numeric, "k1", _TXT("parameter of the BM25pff weighting scheme"), "1:1000");
		rt( P::Numeric, "b", _TXT("parameter of the BM25pff weighting scheme"), "0.0001:1000");
		rt( P::Numeric, "titleinc", _TXT("ff increment for title features"), "0.0:");
		rt( P::Numeric, "tidocnorm", _TXT("specifies a normalization factor of the title weight between 0 and 1. Document bigger or equal this value get close to 1, others smaller"), "0:");
		rt( P::Numeric, "windowsize", _TXT("the size of the window used for finding features to increment proximity scores"), "");
		rt( P::Numeric, "cardinality", _TXT("the number of query features a proximity score window must contain to be considered (optional, default is all features)"), "");		
		rt( P::Numeric, "ffbase", _TXT( "value in the range from 0.0 to 1.0 specifying the percentage of the constant score on the proximity ff for every feature occurrence. (with 1.0 the scheme is plain BM25)"), "0.0:1.0");
		rt( P::Numeric, "fftie", _TXT( "value specifying the mapping of the ff of a weighted to an intervall between 0 and this value"), "0:");
		rt( P::Numeric, "proxffbias", _TXT( "bias for proximity ff increments always counted (the others are counted only till 'proxfftie'"), "0:");
		rt( P::Numeric, "proxfftie", _TXT( "the maximum proximity based ff value that is considered for weighting except for increments exceeding 'proxffbias'"), "0.0:");
		rt( P::Numeric, "avgdoclen", _TXT("the average document lenght"), "0:");
		rt( P::Numeric, "maxdf", _TXT("the maximum df as fraction of the collection size"), "0:");
		rt( P::Metadata, "metadata_doclen", _TXT("the meta data element name referencing the document lenght for each document weighted"), "");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating weighting function description for '%s': %s"), WEIGHTING_SCHEME_NAME, *m_errorhnd, FunctionDescription());
}

