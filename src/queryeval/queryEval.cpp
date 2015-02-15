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
#include "strus/summarizerConfig.hpp"
#include "summarizerDef.hpp"
#include "strus/weightingConfig.hpp"
#include "weightingDef.hpp"
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

#undef STRUS_LOWLEVEL_DEBUG

using namespace strus;

void QueryEval::addTerm(
		const std::string& set_,
		const std::string& type_,
		const std::string& value_)
{
	m_terms.push_back( TermConfig( set_, type_, value_));
}

void QueryEval::addSelectionFeature( const std::string& set_)
{
	m_selectionSets.push_back( set_);
}

void QueryEval::addRestrictionFeature( const std::string& set_)
{
	m_restrictionSets.push_back( set_);
}

void QueryEval::addWeightingFeature( const std::string& set_)
{
	m_weightingSets.push_back( set_);
}

void QueryEval::addSummarizer(
		const std::string& resultAttribute,
		const std::string& functionName,
		const SummarizerConfig& config)
{
	const SummarizerFunctionInterface*
		function = m_processor->getSummarizerFunction( functionName);

	m_summarizers.push_back( SummarizerDef( function, functionName, config));
}

void QueryEval::setWeighting(
		const std::string& functionName,
		const WeightingConfig& config)
{
	const WeightingFunctionInterface*
		function = m_processor->getWeightingFunction( functionName);

	m_weighting = WeightingDef( function, functionName, config);
}

void QueryEval::print( std::ostream& out) const
{
	std::vector<TermConfig>::const_iterator ti = m_terms.begin(), te = m_terms.end();
	for (; ti != te; ++ti)
	{
		out << "TERM " << ti->set << ": " << ti->type << " '" << ti->value << "';" << std::endl;
	}
	if (m_weighting.function())
	{
		out << "EVAL ";
		out << " " << m_weighting.functionName();
		if (m_weighting.parameters().size())
		{
			out << "(";
			std::size_t ai = 0, ae = m_weighting.parameters().size();
			for(; ai != ae; ++ai)
			{
				if (ai) out << ", ";
				out << m_weighting.function()->numericParameterNames()[ai]
					<< "=" << m_weighting.parameters()[ai];
			}
			out << ")";
		}
		if (m_selectionSets.size())
		{
			out << " ON ";
			std::size_t si = 0, se = m_selectionSets.size();
			for(; si != se; ++si)
			{
				if (si) out << ", ";
				out << m_selectionSets[si];
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
	std::vector<SummarizerDef>::const_iterator
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
		std::vector<SummarizerDef::Feature>::const_iterator
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

