/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
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
	try
	{
		m_terms.push_back( TermConfig( set_, type_, value_));
	}
	CATCH_ERROR_MAP( _TXT("error adding term: %s"), *m_errorhnd);
}

void QueryEval::addSelectionFeature( const std::string& set_)
{
	try
	{
		m_selectionSets.push_back( set_);
	}
	CATCH_ERROR_MAP( _TXT("error adding selection feature: %s"), *m_errorhnd);
}

void QueryEval::addRestrictionFeature( const std::string& set_)
{
	try
	{
		m_restrictionSets.push_back( set_);
	}
	CATCH_ERROR_MAP( _TXT("error adding restriction feature: %s"), *m_errorhnd);
}

void QueryEval::addExclusionFeature( const std::string& set_)
{
	try
	{
		m_exclusionSets.push_back( set_);
	}
	CATCH_ERROR_MAP( _TXT("error adding exclusion feature: %s"), *m_errorhnd);
}

void QueryEval::addSummarizerFunction(
		const std::string& functionName,
		SummarizerFunctionInstanceInterface* function,
		const std::vector<FeatureParameter>& featureParameters,
		const std::string& debugAttributeName)
{
	try
	{
		Reference<SummarizerFunctionInstanceInterface> functionref( function);
		defineVariableAssignments(
			functionref->getVariables(),
			VariableAssignment::WeightingFunction,
			m_summarizers.size());
		m_summarizers.push_back( SummarizerDef( functionName, functionref, featureParameters, debugAttributeName));
	}
	CATCH_ERROR_MAP( _TXT("error adding summarization function: %s"), *m_errorhnd);
}

void QueryEval::addWeightingFunction(
		const std::string& functionName,
		WeightingFunctionInstanceInterface* function,
		const std::vector<FeatureParameter>& featureParameters,
		const std::string& debugAttributeName)
{
	try
	{
		Reference<WeightingFunctionInstanceInterface> functionref( function);
		defineVariableAssignments(
			functionref->getVariables(),
			VariableAssignment::WeightingFunction,
			m_weightingFunctions.size());
		m_weightingFunctions.push_back( WeightingDef( functionref, functionName, featureParameters, debugAttributeName));
	}
	CATCH_ERROR_MAP( _TXT("error adding weighting function: %s"), *m_errorhnd);
}

void QueryEval::defineWeightingFormula(
		ScalarFunctionInterface* combinefunc)
{
	try
	{
		m_weightingFormula.reset( combinefunc);
		defineVariableAssignments(
			combinefunc->getVariables(),
			VariableAssignment::FormulaFunction,
			0);
	}
	CATCH_ERROR_MAP( _TXT("error adding weighting formula: %s"), *m_errorhnd);
}

void QueryEval::print( std::ostream& out) const
{
	try
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
			out << si->functionName();
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
	CATCH_ERROR_MAP( _TXT("error printing query evaluation structure: %s"), *m_errorhnd);
}

void QueryEval::defineVariableAssignments( const std::vector<std::string>& variables, VariableAssignment::Target target, std::size_t index)
{
	std::vector<std::string>::const_iterator vi = variables.begin(), ve = variables.end();
	for (; vi != ve; ++vi)
	{
		m_varassignmap.insert( std::pair<std::string, VariableAssignment>( *vi, VariableAssignment( target, index)));
	}
}

std::vector<QueryEval::VariableAssignment> QueryEval::weightingVariableAssignmentList( const std::string& varname) const
{
	std::vector<VariableAssignment> rt;
	typedef std::multimap<std::string,VariableAssignment>::const_iterator map_const_iterator;
	std::pair<map_const_iterator,map_const_iterator> range = m_varassignmap.equal_range( varname);
	map_const_iterator ri = range.first, re = range.second;
	for (; ri != re; ++ri)
	{
		rt.push_back( ri->second);
	}
	return rt;
}

QueryInterface* QueryEval::createQuery( const StorageClientInterface* storage) const
{
	try
	{
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "create query for program:" << std::endl;
		print( std::cout);
#endif
		return new Query( this, storage, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating query: %s"), *m_errorhnd, 0);
}

