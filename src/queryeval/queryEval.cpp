/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#include "queryEval.hpp"
#include "query.hpp"
#include "keyMap.hpp"
#include "termConfig.hpp"
#include "summarizerConfig.hpp"
#include "weightingConfig.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/constants.hpp"
#include "strus/attributeReaderInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/weightingFunctionInterface.hpp"
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/private/arithmeticVariantAsString.hpp"
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/string.hpp>

#define STRUS_LOWLEVEL_DEBUG

using namespace strus;

void QueryEval::defineTerm(
		const std::string& set_,
		const std::string& type_,
		const std::string& value_)
{
	defineTerm( TermConfig( set_, type_, value_));
}

void QueryEval::defineSelectorFeature( const std::string& set_)
{
	m_selectorSets.push_back( set_);
}

void QueryEval::defineWeightingFeature( const std::string& set_)
{
	m_weightingSets.push_back( set_);
}

SummarizerConfigInterface* QueryEval::createSummarizerConfig(
		const std::string& resultAttribute,
		const std::string& functionName)
{
	const SummarizerFunctionInterface* function_ = m_processor->getSummarizerFunction( functionName);
	return new SummarizerConfig( this, function_, functionName);
}

WeightingConfigInterface* QueryEval::createWeightingConfig(
		const std::string& functionName)
{
	const WeightingFunctionInterface* function_ = m_processor->getWeightingFunction( functionName);
	return new WeightingConfig( this, function_, functionName);
}

void QueryEval::defineTerm( const TermConfig& termConfig)
{
	m_terms.push_back( termConfig);
}

void QueryEval::defineWeighting( const WeightingConfig& weightingConfig_)
{
	if (m_weightingConfig.function())
	{
		throw std::runtime_error( "more than one weighting function defined");
	}
	m_weightingConfig = weightingConfig_;
}

void QueryEval::defineSummarizer( const SummarizerConfig& sumDef)
{
	m_summarizers.push_back( sumDef);
}

void QueryEval::print( std::ostream& out) const
{
	std::vector<TermConfig>::const_iterator ti = m_terms.begin(), te = m_terms.end();
	for (; ti != te; ++ti)
	{
		out << "TERM " << ti->set << ": " << ti->type << " '" << ti->value << "';" << std::endl;
	}
	if (m_weightingConfig.function())
	{
		out << "EVAL ";
		out << " " << m_weightingConfig.functionName();
		if (m_weightingConfig.parameters().size())
		{
			out << "(";
			std::size_t ai = 0, ae = m_weightingConfig.parameters().size();
			for(; ai != ae; ++ai)
			{
				if (ai) out << ", ";
				out << m_weightingConfig.function()->numericParameterNames()[ai]
					<< "=" << m_weightingConfig.parameters()[ai];
			}
			out << ")";
		}
		if (m_selectorSets.size())
		{
			out << " ON ";
			std::size_t si = 0, se = m_selectorSets.size();
			for(; si != se; ++si)
			{
				if (si) out << ", ";
				out << m_selectorSets[si];
			}
		}
		if (m_weightingSets.size())
		{
			out << " WITH ";
			std::size_t wi = 0, we = m_weightingSets.size();
			for(; wi != we; ++wi)
			{
				if (wi) out << ", ";
				out << m_weightingSets[wi];
			}
		}
		out << ";" << std::endl;
	}
	std::vector<SummarizerConfig>::const_iterator
		si = m_summarizers.begin(), se = m_summarizers.end();
	for (; si != se; ++si)
	{
		out << "SUMMARIZE ";
		out << si->resultAttribute() << " = " << si->functionName();
		out << "( ";
		std::size_t argidx = 0;

		std::size_t ai = 0, ae = si->numericParameters().size();
		for(; ai != ae; ++ai,++argidx)
		{
			if (argidx) out << ", ";
			out << si->function()->numericParameterNames()[ai]
				<< "=" << si->numericParameters()[ai];
		}
		ai = 0, ae = si->textualParameters().size();
		for(; ai != ae; ++ai,++argidx)
		{
			if (argidx) out << ", ";
			out << si->function()->textualParameterNames()[ai] << "=" << si->textualParameters()[ai];
		}
		std::vector<SummarizerConfig::Feature>::const_iterator
			fi = si->featureParameters().begin(),
			fe = si->featureParameters().end();
		for (int fidx=0; fi != fe; ++fi,++fidx,++argidx)
		{
			if (argidx) out << ", ";
			out << si->function()->featureParameterClassNames()[fi->classidx]
				<< "=" << fi->set;
		}
		out << ");" << std::endl;
	}
}


QueryInterface* QueryEval::createQuery( const StorageInterface* storage) const
{
#ifdef STRUS_LOWLEVEL_DEBUG
	std::cout << "create query for program:" << std::endl;
	print( std::cout);
#endif
	return new Query( this, storage, m_processor);
}

