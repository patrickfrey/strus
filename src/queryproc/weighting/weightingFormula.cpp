/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "weightingFormula.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/utils.hpp"
#include "strus/constants.hpp"
#include <cmath>
#include <ctime>

using namespace strus;


FunctionMap::FunctionMap()
	:FormulaInterpreter::FunctionMap( &dimMap)
{
	defineVariableMap( "df", &variableMap_df);
	defineVariableMap( "ff", &variableMap_ff);
	defineVariableMap( "weight", &variableMap_weight);
	defineUnaryFunction( "log", &unaryFunction_log10);
	defineUnaryFunction( "-", &unaryFunction_minus);
	defineBinaryFunction( "-", &binaryFunction_minus);
	defineBinaryFunction( "+", &binaryFunction_plus);
	defineBinaryFunction( "*", &binaryFunction_mul);
	defineBinaryFunction( "/", &binaryFunction_div);
	defineWeightingFunction( "minwinsize", &weightingFunction_minwinsize);
	defineWeightingFunction( "minwinpos", &weightingFunction_minwinpos);
}

FormulaInterpreter::IteratorSpec FunctionMap::dimMap( void* ctx, const char* type)
{
	WeightingFunctionContextFormula* THIS = (WeightingFunctionContextFormula*)ctx;
	std::map<std::string,std::size_t>::const_iterator ti = THIS->m_sets.find( type);
	if (ti == THIS->m_sets.end()) return FormulaInterpreter::IteratorSpec();
	return FormulaInterpreter::IteratorSpec( ti->second, THIS->m_featar[ ti->second].size());
}

double FunctionMap::variableMap_metadata( void* ctx, int typeidx, unsigned int idx)
{
	WeightingFunctionContextFormula* THIS = (WeightingFunctionContextFormula*)ctx;
	NumericVariant val = THIS->m_metadata->getValue( idx);
	if (val.defined())
	{
		return (double)val;
	}
	else
	{
		return std::numeric_limits<double>::quiet_NaN();
	}
}

double FunctionMap::variableMap_param( void* ctx, int typeidx, unsigned int idx)
{
	WeightingFunctionContextFormula* THIS = (WeightingFunctionContextFormula*)ctx;
	if (idx >= THIS->m_paramar.size()) return std::numeric_limits<double>::quiet_NaN();
	return THIS->m_paramar[idx];
}

double FunctionMap::variableMap_df( void* ctx, int typeidx, unsigned int idx)
{
	WeightingFunctionContextFormula* THIS = (WeightingFunctionContextFormula*)ctx;
	if (typeidx < 0) return std::numeric_limits<double>::quiet_NaN();
	if (idx >= THIS->m_featar[ typeidx].size()) return std::numeric_limits<double>::quiet_NaN();
	return THIS->m_featar[ typeidx][idx].df();
}

double FunctionMap::variableMap_ff( void* ctx, int typeidx, unsigned int idx)
{
	WeightingFunctionContextFormula* THIS = (WeightingFunctionContextFormula*)ctx;
	if (typeidx < 0) return std::numeric_limits<double>::quiet_NaN();
	if (idx >= THIS->m_featar[ typeidx].size()) return std::numeric_limits<double>::quiet_NaN();
	return THIS->m_featar[ typeidx][idx].ff();
}

double FunctionMap::variableMap_weight( void* ctx, int typeidx, unsigned int idx)
{
	WeightingFunctionContextFormula* THIS = (WeightingFunctionContextFormula*)ctx;
	if (typeidx < 0) return std::numeric_limits<double>::quiet_NaN();
	if (idx >= THIS->m_featar[ typeidx].size()) return std::numeric_limits<double>::quiet_NaN();
	return THIS->m_featar[ typeidx][idx].weight();
}

double FunctionMap::variableMap_match( void* ctx, int typeidx, unsigned int idx)
{
	WeightingFunctionContextFormula* THIS = (WeightingFunctionContextFormula*)ctx;
	if (typeidx < 0) return std::numeric_limits<double>::quiet_NaN();
	if (idx >= THIS->m_featar[ typeidx].size()) return std::numeric_limits<double>::quiet_NaN();
	return THIS->m_featar[ typeidx][idx].match();
}

double FunctionMap::unaryFunction_minus( double arg)
{
	return -arg;
}

double FunctionMap::unaryFunction_log10( double arg)
{
	return std::log10( arg);
}

double FunctionMap::binaryFunction_minus( double arg1, double arg2)
{
	return arg1 - arg2;
}

double FunctionMap::binaryFunction_plus( double arg1, double arg2)
{
	return arg1 + arg2;
}

double FunctionMap::binaryFunction_mul( double arg1, double arg2)
{
	return arg1 * arg2;
}

double FunctionMap::binaryFunction_div( double arg1, double arg2)
{
	return arg1 / arg2;
}

double FunctionMap::weightingFunction_minwinsize( void* ctx, int typeidx, int range, int cardinality)
{
	// Code for min window size calculation
	return 0.0;
}

double FunctionMap::weightingFunction_minwinpos( void* ctx, int typeidx, int range, int cardinality)
{
	// Code for min window first position calculation
	return 0.0;
}


WeightingFunctionContextFormula::WeightingFunctionContextFormula(
		const StorageClientInterface* storage,
		MetaDataReaderInterface* metadata_,
		const FormulaInterpreter::FunctionMap& functionMap,
		const std::string& formula,
		const std::vector<double>& paramar,
		ErrorBufferInterface* errorhnd_)
	:m_paramar(paramar)
	,m_featar()
	,m_sets()
	,m_metadata(metadata_)
	,m_interpreter( functionMap, formula)
	,m_errorhnd(errorhnd_)
{}

void WeightingFunctionContextFormula::addWeightingFeature(
		const std::string& name_,
		PostingIteratorInterface* itr_,
		float weight_,
		const TermStatistics& stats_)
{
	try
	{
		std::size_t idx;
		std::map<std::string,std::size_t>::const_iterator ti = m_sets.find( name_);
		if (ti == m_sets.end())
		{
			m_sets[ name_] = idx = m_featar.size();
			m_featar.push_back( FeatureVector());
		}
		else
		{
			idx = ti->second;
		}
		m_featar[ idx].push_back( Feature( itr_, weight_, stats_));
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding feature to weighting function '%s': %s"), "formula", *m_errorhnd);
}

double WeightingFunctionContextFormula::call( const Index& docno)
{
	std::vector<FeatureVector>::iterator vi = m_featar.begin(), ve = m_featar.end();
	for (; vi != ve; ++vi)
	{
		FeatureVector::iterator fi = vi->begin(), fe = vi->end();
		for (; fi != fe; ++fi)
		{
			fi->skipDoc( docno);
		}
	}
	try
	{
		m_metadata->skipDoc( docno);
		return m_interpreter.run( this);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error calling weighting function '%s': %s"), "formula", *m_errorhnd, 0.0);
}


WeightingFunctionContextInterface* WeightingFunctionInstanceFormula::createFunctionContext(
		const StorageClientInterface* storage_,
		MetaDataReaderInterface* metadata,
		const GlobalStatistics& stats) const
{
	try
	{
		FormulaInterpreter::FunctionMap funcmap( m_functionmap);
		std::vector<double> paramar( m_paramar);
		Index ii=0,nn = metadata->nofElements();
		for (;ii<nn;++ii)
		{
			const char* name = metadata->getName( ii);
			funcmap.defineVariableMap( name, FormulaInterpreter::VariableMap( &FunctionMap::variableMap_metadata, ii));
		}
		GlobalCounter nofdocs = stats.nofDocumentsInserted()>=0?stats.nofDocumentsInserted():(GlobalCounter)storage_->nofDocumentsInserted();
		funcmap.defineVariableMap( "nofdocs", FormulaInterpreter::VariableMap( &FunctionMap::variableMap_param, m_paramar.size()));
		paramar.push_back( (double)nofdocs);

		if (m_formula.empty())
		{
			throw strus::runtime_error(_TXT("no weighting formula defined with string parameter 'formula'"));
		}
		return new WeightingFunctionContextFormula( storage_, metadata, funcmap, m_formula, paramar, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of weighting function '%s': %s"), "formula", *m_errorhnd, 0);
}

void WeightingFunctionInstanceFormula::addStringParameter( const std::string& name, const std::string& value)
{
	try
	{
		if (utils::caseInsensitiveEquals( name, "formula"))
		{
			m_formula = value;
			if (value.empty()) throw strus::runtime_error( _TXT("empty value passed as '%s' weighting function parameter '%s'"), "formula", name.c_str());
		}
		else
		{
			throw strus::runtime_error( _TXT("unknown '%s' weighting function parameter '%s'"), "formula", name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding string parameter to weighting function '%s': %s"), "formula", *m_errorhnd);
}

void WeightingFunctionInstanceFormula::addNumericParameter( const std::string& name, const NumericVariant& value)
{
	try
	{
		m_functionmap.defineVariableMap( name, FormulaInterpreter::VariableMap( &FunctionMap::variableMap_param, m_paramar.size()));
		m_paramar.push_back( (double)value);
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding numeric parameter to weighting function '%s': %s"), "formula", *m_errorhnd);
}

std::string WeightingFunctionInstanceFormula::tostring() const
{
	try
	{
		return m_formula + "\n--\n" + m_functionmap.tostring();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error mapping weighting function '%s' to string: %s"), "formula", *m_errorhnd, std::string());
}


WeightingFunctionInstanceInterface* WeightingFunctionFormula::createInstance(
		const QueryProcessorInterface*) const
{
	try
	{
		return new WeightingFunctionInstanceFormula( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of weighting function '%s': %s"), "formula", *m_errorhnd, 0);
}

FunctionDescription WeightingFunctionFormula::getDescription() const
{
	try
	{
		typedef FunctionDescription::Parameter P;
		FunctionDescription rt( _TXT("Calculate the weight of a document with a formula"));
		rt( P::Feature, "[a-z]+", _TXT( "defines the query features referenced in the formula to weight"), "");
		rt( P::String, "formula", _TXT( "defines an expression to evaluate. You can use the operators '*','/','+','-' and the functions 'log'. Mixing operators of different precedence is only allowed using brackets '(' and ')'. The variables 'weight','ff' and 'df' can be used besides all variables specified as parameters or as meta data elements"), "");
		rt( P::Numeric, "[a-z]+", _TXT( "defines a variable to be used in the formula expression"), "");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating weighting function description for '%s': %s"), "formula", *m_errorhnd, FunctionDescription());
}

