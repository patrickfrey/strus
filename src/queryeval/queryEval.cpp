/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "queryEval.hpp"
#include "query.hpp"
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
#include <algorithm>

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
		if (std::find( m_selectionSets.begin(), m_selectionSets.end(), set_) == m_selectionSets.end())
		{
			m_selectionSets.push_back( set_);
		}
	}
	CATCH_ERROR_MAP( _TXT("error adding selection feature: %s"), *m_errorhnd);
}

void QueryEval::addRestrictionFeature( const std::string& set_)
{
	try
	{
		if (std::find( m_restrictionSets.begin(), m_restrictionSets.end(), set_) == m_restrictionSets.end())
		{
			m_restrictionSets.push_back( set_);
		}
	}
	CATCH_ERROR_MAP( _TXT("error adding restriction feature: %s"), *m_errorhnd);
}

void QueryEval::addExclusionFeature( const std::string& set_)
{
	try
	{
		if (std::find( m_exclusionSets.begin(), m_exclusionSets.end(), set_) == m_exclusionSets.end())
		{
			m_exclusionSets.push_back( set_);
		}
	}
	CATCH_ERROR_MAP( _TXT("error adding exclusion feature: %s"), *m_errorhnd);
}

std::vector<std::string> QueryEval::getWeightingFeatureSets() const
{
	try
	{
		return m_weightingSets;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error getting the weighting feature sets: %s"), *m_errorhnd, std::vector<std::string>());
}

std::vector<std::string> QueryEval::getSelectionFeatureSets() const
{
	try
	{
		return m_selectionSets;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error getting the selection feature sets: %s"), *m_errorhnd, std::vector<std::string>());
}

std::vector<std::string> QueryEval::getRestrictionFeatureSets() const
{
	try
	{
		return m_restrictionSets;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error getting the restriction feature sets: %s"), *m_errorhnd, std::vector<std::string>());
}

std::vector<std::string> QueryEval::getExclusionFeatureSets() const
{
	try
	{
		return m_exclusionSets;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error getting the exclusion feature sets: %s"), *m_errorhnd, std::vector<std::string>());
}

void QueryEval::addSummarizerFunction(
		const std::string& summaryId,
		SummarizerFunctionInstanceInterface* function,
		const std::vector<FeatureParameter>& featureParameters,
		const std::string& debugAttributeName)
{
	try
	{
		Reference<SummarizerFunctionInstanceInterface> functionref( function);
		defineVariableAssignments(
			functionref->getVariables(),
			VariableAssignment::SummarizerFunction,
			m_summarizers.size());
		m_summarizers.push_back( SummarizerDef( summaryId, functionref, featureParameters, debugAttributeName));
	}
	CATCH_ERROR_MAP( _TXT("error adding summarization function: %s"), *m_errorhnd);
}

void QueryEval::addWeightingFunction(
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
		std::vector<FeatureParameter>::const_iterator fi = featureParameters.begin(), fe = featureParameters.end();
		for (; fi != fe; ++fi)
		{
			if (std::find( m_weightingSets.begin(), m_weightingSets.end(), fi->featureSet()) == m_weightingSets.end())
			{
				m_weightingSets.push_back( fi->featureSet());
			}
		}
		m_weightingFunctions.push_back( WeightingDef( functionref, featureParameters, debugAttributeName));
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

void QueryEval::usePositionInformation( const std::string& featureSet, bool yes)
{
	try
	{
		m_featureSetFlagMap[ featureSet].usePosinfo = yes;
	}
	CATCH_ERROR_MAP( _TXT("error initializing position info flag: %s"), *m_errorhnd);
}

bool QueryEval::usePositionInformation( const std::string& featureSet) const
{
	std::map<std::string,FeatureSetFlags>::const_iterator fi = m_featureSetFlagMap.find( featureSet);
	return fi == m_featureSetFlagMap.end() ? true : fi->second.usePosinfo;
}

static StructView getStructView( const std::vector<QueryEvalInterface::FeatureParameter>& o)
{
	StructView rt;
	std::vector<QueryEvalInterface::FeatureParameter>::const_iterator oi = o.begin(), oe = o.end();
	for (; oi != oe; ++oi)
	{
		rt( oi->featureRole(), oi->featureSet() );
	}
	return rt;
}

static StructView getStructView( const WeightingDef& o)
{
	StructView rt;
	rt( "function", o.function()->name());
	if (o.function())
	{
		StructView param( o.function()->view());
		if (!o.debugAttributeName().empty()) param( "debug", o.debugAttributeName());
		rt( "param", param);
	}
	if (!o.featureParameters().empty())
	{
		rt( "feature", getStructView( o.featureParameters()));
	}
	return rt;
}

static StructView getStructView( const std::vector<WeightingDef>& o)
{
	StructView rt;
	std::vector<WeightingDef>::const_iterator oi = o.begin(), oe = o.end();
	for (; oi != oe; ++oi)
	{
		rt( getStructView( *oi));
	}
	return rt;
}

static StructView getStructView( const SummarizerDef& o)
{
	StructView rt;
	rt( "function", o.function()->name());
	if (o.function())
	{
		StructView param( o.function()->view());
		if (!o.debugAttributeName().empty()) param( "debug", o.debugAttributeName());
		rt( "param", param);
	}
	if (!o.featureParameters().empty())
	{
		rt( "feature", getStructView( o.featureParameters()));
	}
	return rt;
}

static StructView getStructView( const std::vector<SummarizerDef>& o)
{
	StructView rt;
	std::vector<SummarizerDef>::const_iterator oi = o.begin(), oe = o.end();
	for (; oi != oe; ++oi)
	{
		rt( getStructView( *oi));
	}
	return rt;
}

static StructView getStructView( const TermConfig& o)
{
	return StructView()
		( "set", o.set)
		( "type", o.type)
		( "value", o.value);
}

static StructView getStructView( const std::vector<TermConfig>& o)
{
	StructView rt;
	std::vector<TermConfig>::const_iterator oi = o.begin(), oe = o.end();
	for (; oi != oe; ++oi)
	{
		rt( getStructView( *oi));
	}
	return rt;
}

StructView QueryEval::view() const
{
	try
	{
		StructView rt;
		if (!m_weightingFunctions.empty())
		{
			rt( "weighting", getStructView( m_weightingFunctions));
		}
		if (!m_summarizers.empty())
		{
			rt( "summarizers", getStructView( m_summarizers));
		}
		if (!m_terms.empty())
		{
			rt( "weighting_sets", getStructView( m_terms));
		}
		if (!m_weightingSets.empty())
		{
			rt( "weighting_sets", m_weightingSets);
		}
		if (!m_selectionSets.empty())
		{
			rt( "selection_sets", m_selectionSets);
		}
		if (!m_restrictionSets.empty())
		{
			rt( "restriction_sets", m_restrictionSets);
		}
		if (!m_exclusionSets.empty())
		{
			rt( "exclusion_sets", m_exclusionSets);
		}
		if (m_weightingFormula.get())
		{
			rt( "formula", m_weightingFormula->view());
		}
		return rt;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating query: %s"), *m_errorhnd, StructView());
}


