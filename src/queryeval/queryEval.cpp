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
#include "summarizerDef.hpp"
#include "weightingDef.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/constants.hpp"
#include "strus/attributeReaderInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/private/arithmeticVariantAsString.hpp"
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <iomanip>

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

void QueryEval::addExclusionFeature( const std::string& set_)
{
	m_exclusionSets.push_back( set_);
}

void QueryEval::addSummarizerFunction(
		const std::string& functionName,
		SummarizerFunctionInstanceInterface* function,
		const std::vector<FeatureParameter>& featureParameters,
		const std::string& resultAttribute)
{
	m_summarizers.push_back( SummarizerDef( resultAttribute, functionName, function, featureParameters));
}

void QueryEval::addWeightingFunction(
		const std::string& functionName,
		WeightingFunctionInstanceInterface* function,
		const std::vector<FeatureParameter>& featureParameters,
		float weight)
{
	m_weightingFunctions.push_back( WeightingDef( function, functionName, featureParameters, weight));
}

void QueryEval::print( std::ostream& out) const
{
	std::vector<TermConfig>::const_iterator
		ti = m_terms.begin(), te = m_terms.end();
	for (; ti != te; ++ti)
	{
		out << "TERM " << ti->set << ": " << ti->type << " '" << ti->value << "';" << std::endl;
	}
	if (m_selectionSets.size())
	{
		out << "SELECT ";
		std::size_t si = 0, se = m_selectionSets.size();
		for(; si != se; ++si)
		{
			if (si) out << ", ";
			out << m_selectionSets[si];
		}
		out << ";" << std::endl;
	}
	if (m_restrictionSets.size())
	{
		out << "RESTRICT ";
		std::size_t si = 0, se = m_restrictionSets.size();
		for(; si != se; ++si)
		{
			if (si) out << ", ";
			out << m_restrictionSets[si];
		}
		out << ";" << std::endl;
	}
	std::vector<WeightingDef>::const_iterator
		fi = m_weightingFunctions.begin(), fe = m_weightingFunctions.end();
	for (; fi != fe; ++fi)
	{
		out << "EVAL ";
		std::string params = fi->function()->tostring();
		out << " " << fi->functionName() << "( " << params;
		std::vector<FeatureParameter>::const_iterator
			pi = fi->featureParameters().begin(), pe = fi->featureParameters().end();
		int pidx = params.size();
		for (; pi != pe; ++pi,++pidx)
		{
			if (pidx) out << ", ";
			out << pi->parameterName() << "= %" << pi->featureSet();
		}
		out << ");" << std::endl;
	}
	std::vector<SummarizerDef>::const_iterator
		si = m_summarizers.begin(), se = m_summarizers.end();
	for (; si != se; ++si)
	{
		out << "SUMMARIZE ";
		out << si->resultAttribute() << " = " << si->functionName();
		std::string params = si->function()->tostring();
		out << "( " << params;

		std::vector<FeatureParameter>::const_iterator
			fi = si->featureParameters().begin(),
			fe = si->featureParameters().end();
		int fidx=params.size();
		for (; fi != fe; ++fi,++fidx)
		{
			if (fidx) out << ", ";
			out << fi->parameterName() << "= %" << fi->featureSet();
		}
		out << ");" << std::endl;
	}
}


QueryInterface* QueryEval::createQuery( const StorageClientInterface* storage) const
{
#ifdef STRUS_LOWLEVEL_DEBUG
	std::cout << "create query for program:" << std::endl;
	print( std::cout);
#endif
	return new Query( this, storage);
}

