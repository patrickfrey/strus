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
#include "weightingFormula.hpp"
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
	ArithmeticVariant val = THIS->m_metadata->getValue( idx);
	if (val.defined())
	{
		return (double)(float)val;
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
	return std::log( arg);
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


WeightingFunctionContextFormula::WeightingFunctionContextFormula(
		const StorageClientInterface* storage,
		MetaDataReaderInterface* metadata_,
		const FormulaInterpreter::FunctionMap& functionMap,
		const std::string& formula,
		const std::vector<double>& paramar)
	:m_paramar(paramar)
	,m_featar()
	,m_sets()
	,m_metadata(metadata_)
	,m_interpreter( functionMap, formula)
{
	
}

void WeightingFunctionContextFormula::addWeightingFeature(
		const std::string& name_,
		PostingIteratorInterface* itr_,
		float weight_)
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
	m_featar[ idx].push_back( Feature( itr_, weight_));
}

float WeightingFunctionContextFormula::call( const Index& docno)
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
	return m_interpreter.run( this);
}


WeightingFunctionContextInterface* WeightingFunctionInstanceFormula::createFunctionContext(
		const StorageClientInterface* storage_,
		MetaDataReaderInterface* metadata) const
{
	FormulaInterpreter::FunctionMap funcmap( m_functionmap);
	std::vector<double> paramar( m_paramar);
	Index ii=0,nn = metadata->nofElements();
	for (;ii<nn;++ii)
	{
		const char* name = metadata->getName( ii);
		funcmap.defineVariableMap( name, FormulaInterpreter::VariableMap( &FunctionMap::variableMap_metadata, ii));
	}
	funcmap.defineVariableMap( "nofdocs", FormulaInterpreter::VariableMap( &FunctionMap::variableMap_param, m_paramar.size()));
	paramar.push_back( (double)(storage_->globalNofDocumentsInserted()));

	if (m_formula.empty())
	{
		throw strus::runtime_error(_TXT("no weighting formula defined with string parameter 'formula'"));
	}
	return new WeightingFunctionContextFormula( storage_, metadata, funcmap, m_formula, paramar);
}

void WeightingFunctionInstanceFormula::addStringParameter( const std::string& name, const std::string& value)
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

void WeightingFunctionInstanceFormula::addNumericParameter( const std::string& name, const ArithmeticVariant& value)
{
	m_functionmap.defineVariableMap( name, FormulaInterpreter::VariableMap( &FunctionMap::variableMap_param, m_paramar.size()));
	m_paramar.push_back( (double)(float)value);
}

