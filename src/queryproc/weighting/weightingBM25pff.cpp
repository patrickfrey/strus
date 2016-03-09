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
		double k1_,
		double b_,
		unsigned int windowsize_,
		unsigned int cardinality_,
		double ffbase_,
		double maxdf_,
		double avgDocLength_,
		double titleinc_,
		double nofCollectionDocuments_,
		const std::string& metadata_doclen_,
		const std::string& metadata_title_maxpos_,
		const std::string& metadata_title_size_,
		ErrorBufferInterface* errorhnd_)
	:m_k1(k1_),m_b(b_)
	,m_windowsize(windowsize_)
	,m_cardinality(cardinality_)
	,m_ffbase(ffbase_)
	,m_maxdf(maxdf_)
	,m_avgDocLength(avgDocLength_)
	,m_titleinc(titleinc_)
	,m_nofCollectionDocuments(nofCollectionDocuments_)
	,m_itrarsize(0)
	,m_structarsize(0)
	,m_paraarsize(0)
	,m_nof_maxdf_features(0)
	,m_initialized(false)
	,m_metadata(metadata_)
	,m_metadata_doclen(metadata_->elementHandle( metadata_doclen_.empty()?std::string("doclen"):metadata_doclen_))
	,m_metadata_title_maxpos(metadata_title_maxpos_.empty()?-1:metadata_->elementHandle( metadata_title_maxpos_))
	,m_metadata_title_size(metadata_title_size_.empty()?-1:metadata_->elementHandle( metadata_title_size_))
	,m_errorhnd(errorhnd_)
{}

void WeightingFunctionContextBM25pff::addWeightingFeature(
		const std::string& name,
		PostingIteratorInterface* itr,
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
			double idf = std::log10( (m_nofCollectionDocuments - df + 0.5) / (df + 0.5));
			if (m_maxdf * m_nofCollectionDocuments < df)
			{
				m_maxdist_featar[ m_itrarsize] = (m_windowsize > 5)?5:m_windowsize;
				++m_nof_maxdf_features;
			}
			else
			{
				m_maxdist_featar[ m_itrarsize] = m_windowsize;
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
		const Index& firstpos,
		const Index& titlesize,
		double titleweight,
		const ProximityWeightAccumulator::WeightArray& weightincr,
		PostingIteratorInterface** itrar, std::size_t itrarsize)
{
	unsigned long poset = 0;
	Index nofFeaturesInTitle = 0;
	for (std::size_t ii=0; ii<itrarsize; ++ii)
	{
		Index pos = itrar[ii]->skipPos(0);
		if (pos && pos < firstpos)
		{
			if ((poset & (1<<pos)) == 0)
			{
				poset |= (1<<pos);
				++nofFeaturesInTitle;
			}
		}
	}
	if (titlesize < nofFeaturesInTitle)
	{
		nofFeaturesInTitle = titlesize;
	}
	double nf = 1.0 / (double)(titlesize - nofFeaturesInTitle + 1);

	for (std::size_t ii=0; ii<itrarsize; ++ii)
	{
		Index pos = itrar[ii]->skipPos(0);
		if (pos && pos < firstpos)
		{
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "weighting title weight " << titleweight << " incr " << weightincr[ii] << " norm " << nf << " => " << (titleweight * weightincr[ii] * nf) << std::endl;
#endif
			result[ ii] += titleweight * weightincr[ii] * nf;
		}
	}
}

static void calcProximityFfIncrements(
			ProximityWeightAccumulator::WeightArray& result,
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

		// Calculate the ff increment for the current window and add it to the result:
		ProximityWeightAccumulator::weight_same_sentence(
			result, 0.3 * normfactor, weightincr, window, windowsize, maxdist_featar, itrar, itrarsize, structar, structarsize);
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "\taccumulated ff incr [same sentence] " << result.tostring() << std::endl;
#endif
		ProximityWeightAccumulator::weight_imm_follow(
			result, 0.4 * normfactor, weightincr, window, windowsize, itrar, itrarsize);
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "\taccumulated ff incr [imm follow] " << result.tostring() << std::endl;
#endif
		ProximityWeightAccumulator::weight_invdist(
			result, 0.3 * normfactor, weightincr, window, windowsize, itrar, itrarsize);
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "\taccumulated ff incr [inv distance] " << result.tostring() << std::endl;
#endif
		if (windowpos - firstpos < 1000)
		{
			// Weight inv distance from start of document:
			ProximityWeightAccumulator::weight_invpos(
				result, 0.5 * normfactor, weightincr, firstpos, itrar, itrarsize);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "\taccumulated ff incr [inv distance begin] " << result.tostring() << std::endl;
#endif
		}
		if (prevPara && windowpos >= prevPara)
		{
			// Weight inv distance to last paragraph start:
			ProximityWeightAccumulator::weight_invpos(
				result, 0.3 * normfactor, weightincr, prevPara, itrar, itrarsize);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "\taccumulated ff incr [inv distance paragraph] " << result.tostring() << std::endl;
#endif
		}
	}
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
			if (m_itrarsize < m_cardinality || m_cardinality == 0)
			{
				m_cardinality = m_itrarsize;
			}
			if (m_nof_maxdf_features >= m_cardinality)
			{
				m_cardinality = m_nof_maxdf_features+1;
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

		// Define search start position:
		Index firstpos = 1;
		Index titlesize = 1;
		if (m_metadata_title_maxpos>=0)
		{
			ArithmeticVariant firstposval = m_metadata->getValue( m_metadata_title_maxpos);
			firstpos = firstposval.toint()+1;
		}
		if (m_metadata_title_size>=0)
		{
			ArithmeticVariant titlesizeval = m_metadata->getValue( m_metadata_title_size);
			titlesize = titlesizeval.toint();
			if (!titlesize) titlesize = 1;
		}
		else if (m_metadata_title_maxpos>=0)
		{
			titlesize = (firstpos > 1) ? (firstpos - 1):1;
		}
		// Define the structure of accumulated proximity weights:
		ProximityWeightAccumulator::WeightArray ffincrar( m_itrarsize);

		// Calculate ff title increment weights:
		calcTitleFfIncrements(
			ffincrar, firstpos, titlesize, m_titleinc, m_weightincr, m_itrar, m_itrarsize);
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "accumulated ff incr [title terms] " << ffincrar.tostring() << std::endl;
#endif

		// Build array of title terms:
		PostingIteratorInterface* titleTerms[ MaxNofArguments];
		std::size_t nofTitleTerms = 0;
		std::size_t ti=0,te=m_itrarsize;
		for (; ti < te; ++ti)
		{
			Index pos = m_itrar[ti]->skipPos( 0);
			if (pos < firstpos)
			{
				titleTerms[ nofTitleTerms++] = m_itrar[ ti];
			}
		}

		// Calculate ff proximity increment weights for all features:
		if (m_cardinality <= m_itrarsize)
		{
			calcProximityFfIncrements(
				ffincrar, 1, m_weightincr,  m_windowsize, m_cardinality, m_maxdist_featar, m_normfactorar,
				m_itrar, m_itrarsize, m_structar, m_structarsize, m_paraar, m_paraarsize, 0, 0);
			// Calculate ff proximity increment weights for all non title features:
			if (nofTitleTerms && m_itrarsize > nofTitleTerms)
			{
				unsigned int notitle_cardinality = m_itrarsize - nofTitleTerms;
				if (m_cardinality < notitle_cardinality)
				{
					notitle_cardinality = m_cardinality;
				}
#ifdef STRUS_LOWLEVEL_DEBUG
				std::cout << "do second pass without title terms:" << std::endl;
#endif
				calcProximityFfIncrements(
					ffincrar, firstpos, m_weightincr,  m_windowsize, notitle_cardinality,
					m_maxdist_featar, m_normfactorar,
					m_itrar, m_itrarsize, m_structar, m_structarsize,
					m_paraar, m_paraarsize, titleTerms, nofTitleTerms);
			}
		}
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "final accumulated ff increments: " << ffincrar.tostring() << std::endl;
#endif
		double rt = 0.0;
		std::size_t fi = 0;
		for ( ;fi != m_itrarsize; ++fi)
		{
			double ff = m_itrar[ fi]->frequency();
			if (ff <= std::numeric_limits<double>::epsilon()) continue;

			double prox_ff = m_ffbase * ff
					+ (1.0-m_ffbase) * ffincrar[ fi];
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "proximity ff [" << m_itrar[ fi]->featureid() << "] ff=" << ff << " proximity ff=" << prox_ff << std::endl;
#endif
			if (m_b)
			{
				double doclen = m_metadata->getValue( m_metadata_doclen);
				double rel_doclen = (doclen+1) / m_avgDocLength;
#ifdef STRUS_LOWLEVEL_DEBUG
				double ww = m_idfar[ fi]
						* (ff * (m_k1 + 1.0))
						/ (ff + m_k1 * (1.0 - m_b + m_b * rel_doclen));
				std::cout << "idf[" << fi << "]=" << m_idfar[ fi] << " doclen=" << doclen << " weight=" << ww << std::endl;
#endif
				rt += m_idfar[ fi]
					* (prox_ff * (m_k1 + 1.0))
					/ (prox_ff + m_k1 * (1.0 - m_b + m_b * rel_doclen));
			}
			else
			{
#ifdef STRUS_LOWLEVEL_DEBUG
				double ww = m_idfar[ fi]
						* (ff * (m_k1 + 1.0))
						/ (ff + m_k1 * 1.0);
				std::cout << "idf[" << fi << "]=" << m_idfar[ fi] << " weight=" << ww << std::endl;
#endif
				rt += m_idfar[ fi]
					* (prox_ff * (m_k1 + 1.0))
					/ (prox_ff + m_k1 * 1.0);
			}
		}
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "calculated weight " << rt << ", k1=" << m_k1 << ", b=" << m_b << " ,avgdoclen=" << m_avgDocLength << std::endl;
#endif
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error calling weighting function '%s': %s"), WEIGHTING_SCHEME_NAME, *m_errorhnd, 0.0);
}

static ArithmeticVariant parameterValue( const std::string& name, const std::string& value)
{
	ArithmeticVariant rt;
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
		else if (utils::caseInsensitiveEquals( name, "metadata_title_maxpos"))
		{
			m_metadata_title_maxpos = value;
			if (value.empty()) m_errorhnd->report( _TXT("empty value passed as '%s' weighting function parameter '%s'"), WEIGHTING_SCHEME_NAME, name.c_str());
		}
		else if (utils::caseInsensitiveEquals( name, "metadata_title_size"))
		{
			m_metadata_title_size = value;
			if (value.empty()) m_errorhnd->report( _TXT("empty value passed as '%s' weighting function parameter '%s'"), WEIGHTING_SCHEME_NAME, name.c_str());
		}
		else if (utils::caseInsensitiveEquals( name, "k1")
		||  utils::caseInsensitiveEquals( name, "b")
		||  utils::caseInsensitiveEquals( name, "avgdoclen")
		||  utils::caseInsensitiveEquals( name, "titleinc")
		||  utils::caseInsensitiveEquals( name, "windowsize")
		||  utils::caseInsensitiveEquals( name, "cardinality")
		||  utils::caseInsensitiveEquals( name, "ffbase"))
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

void WeightingFunctionInstanceBM25pff::addNumericParameter( const std::string& name, const ArithmeticVariant& value)
{
	if (utils::caseInsensitiveEquals( name, "match") || utils::caseInsensitiveEquals( name, "struct") || utils::caseInsensitiveEquals( name, "para"))
	{
		m_errorhnd->report( _TXT("parameter '%s' for weighting scheme '%s' expected to be defined as feature and not as string or numeric value"), name.c_str(), WEIGHTING_SCHEME_NAME);
	}
	else if (utils::caseInsensitiveEquals( name, "k1"))
	{
		m_k1 = (double)value;
	}
	else if (utils::caseInsensitiveEquals( name, "b"))
	{
		m_b = (double)value;
	}
	else if (utils::caseInsensitiveEquals( name, "avgdoclen"))
	{
		m_avgdoclen = (double)value;
	}
	else if (utils::caseInsensitiveEquals( name, "windowsize"))
	{
		if (value.type == ArithmeticVariant::Int && value.toint() > 0)
		{
			m_windowsize = value.touint();
		}
		else if (value.type == ArithmeticVariant::UInt && value.touint() > 0)
		{
			m_windowsize = value.touint();
		}
		else
		{
			m_errorhnd->report( _TXT("parameter '%s' for weighting scheme '%s' expected to a positive integer value"), name.c_str(), WEIGHTING_SCHEME_NAME);
		}
	}
	else if (utils::caseInsensitiveEquals( name, "cardinality"))
	{
		if (value.type == ArithmeticVariant::Int && value.toint() >= 0)
		{
			m_cardinality = value.touint();
		}
		else if (value.type == ArithmeticVariant::UInt && value.touint() >= 0)
		{
			m_cardinality = value.touint();
		}
		else
		{
			m_errorhnd->report( _TXT("parameter '%s' for weighting scheme '%s' expected to a non negative integer value"), name.c_str(), WEIGHTING_SCHEME_NAME);
		}
	}
	else if (utils::caseInsensitiveEquals( name, "ffbase"))
	{
		m_ffbase = (double)value;
		if (m_ffbase < 0.0 || m_ffbase > 1.0)
		{
			m_errorhnd->report( _TXT("parameter '%s' for weighting scheme '%s' expected to a positive floating point number between 0.0 and 1.0"), name.c_str(), WEIGHTING_SCHEME_NAME);
		}
	}
	else if (utils::caseInsensitiveEquals( name, "maxdf"))
	{
		m_maxdf = (double)value;
		if (m_maxdf < 0.0 || m_maxdf > 1.0)
		{
			m_errorhnd->report( _TXT("parameter '%s' for weighting scheme '%s' expected to a positive floating point number between 0.0 and 1.0"), name.c_str(), WEIGHTING_SCHEME_NAME);
		}
	}
	else if (utils::caseInsensitiveEquals( name, "titleinc"))
	{
		m_titleinc = (double)value;
		if (m_titleinc < 0.0)
		{
			m_errorhnd->report( _TXT("parameter '%s' for weighting scheme '%s' expected to a positive floating point number"), name.c_str(), WEIGHTING_SCHEME_NAME);
		}
	}
	else if (utils::caseInsensitiveEquals( name, "metadata_doclen")
		|| utils::caseInsensitiveEquals( name, "metadata_title_maxpos")
		|| utils::caseInsensitiveEquals( name, "metadata_title_size"))
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
		return new WeightingFunctionContextBM25pff( storage_, metadata, m_k1, m_b, m_windowsize, m_cardinality, m_ffbase, m_maxdf, m_avgdoclen, m_titleinc, nofdocs, m_metadata_doclen, m_metadata_title_maxpos, m_metadata_title_size, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' weighting function: %s"), WEIGHTING_SCHEME_NAME, *m_errorhnd, 0);
}

std::string WeightingFunctionInstanceBM25pff::tostring() const
{
	
	try
	{
		std::ostringstream rt;
		rt << std::setw(2) << std::setprecision(5)
			<< "b=" << m_b << ", k1=" << m_k1 << ", avgdoclen=" << m_avgdoclen;
		return rt.str();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error mapping '%s' weighting function to string: %s"), WEIGHTING_SCHEME_NAME, *m_errorhnd, std::string());
}


WeightingFunctionInstanceInterface* WeightingFunctionBM25pff::createInstance() const
{
	try
	{
		return new WeightingFunctionInstanceBM25pff( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' function instance: %s"), WEIGHTING_SCHEME_NAME, *m_errorhnd, 0);
}

WeightingFunctionInterface::Description WeightingFunctionBM25pff::getDescription() const
{
	try
	{
		Description rt(_TXT("Calculate the document weight with the weighting scheme \"BM25pff\". This is \"BM25\" where the feature frequency is counted by 1.0 per feature only for features with the maximum proximity score. The proximity score is a measure that takes the proximity of other query features into account"));
		rt( Description::Param::Feature, "match", _TXT( "defines the query features to weight"), "");
		rt( Description::Param::Feature, "struct", _TXT( "defines the delimiter for structures"), "");
		rt( Description::Param::Feature, "para", _TXT( "defines the delimiter for paragraphs (windows used for proximity weighting must not overlap paragraph borders)"), "");
		rt( Description::Param::Numeric, "k1", _TXT("parameter of the BM25pff weighting scheme"), "1:1000");
		rt( Description::Param::Numeric, "b", _TXT("parameter of the BM25pff weighting scheme"), "0.0001:1000");
		rt( Description::Param::Numeric, "titleinc", _TXT("ff increment for title features"), "0.0:");
		rt( Description::Param::Numeric, "windowsize", _TXT("the size of the window used for finding features to increment proximity scores"), "");
		rt( Description::Param::Numeric, "cardinality", _TXT("the number of query features a proximity score window must contain to be considered (optional, default is all features)"), "");		
		rt( Description::Param::Metadata, "metadata_title_maxpos", _TXT( "the metadata element that specifies the last title element. Elements in title are scored with an ff increment"), "");
		rt( Description::Param::Metadata, "metadata_title_size", _TXT( "the metadata element that specifies the number of terms (size) of the title"), "");
		rt( Description::Param::Numeric, "ffbase", _TXT( "value in the range from 0.0 to 1.0 specifying the percentage of the constant score on the proximity ff for every feature occurrence. (with 1.0 the scheme is plain BM25)"), "0.0:1.0");
		rt( Description::Param::Numeric, "avgdoclen", _TXT("the average document lenght"), "0:");
		rt( Description::Param::Numeric, "maxdf", _TXT("the maximum df as fraction of the collection size"), "0:");
		rt( Description::Param::Metadata, "metadata_doclen", _TXT("the meta data element name referencing the document lenght for each document weighted"), "");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating weighting function description for '%s': %s"), WEIGHTING_SCHEME_NAME, *m_errorhnd, WeightingFunctionInterface::Description());
}

