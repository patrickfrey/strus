/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
///\file weightingScalar.cpp
///\brief Implementation of a weighting function defined as function on tf,df,N and some metadata references in a string
#include "weightingScalar.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/scalarFunctionInstanceInterface.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/string_conv.hpp"
#include "private/errorUtils.hpp"
#include "private/internationalization.hpp"
#include "private/functionDescription.hpp"
#include "viewUtils.hpp"
#include <sstream>
#include <iostream>
#include <iomanip>

#define THIS_METHOD_NAME const_cast<char*>("scalar")
#define NOF_IMPLICIT_ARGUMENTS 1

using namespace strus;

WeightingFunctionContextScalar::WeightingFunctionContextScalar(
		const ScalarFunctionInterface* func_,
		MetaDataReaderInterface* metadata_,
		const std::vector<Index>& metadatahnd_,
		double nofCollectionDocuments_,
		ErrorBufferInterface* errorhnd_)
	:m_func(func_->createInstance())
	,m_metadata(metadata_)
	,m_metadatahnd(metadatahnd_)
	,m_nofCollectionDocuments(nofCollectionDocuments_)
	,m_lastResult()
	,m_errorhnd(errorhnd_)
{
	if (!m_func.get())
	{
		throw std::runtime_error( _TXT("failed to create weighting function instance"));
	}
}

void WeightingFunctionContextScalar::addWeightingFeature(
		const std::string&,
		PostingIteratorInterface* itr_,
		double weight_)
{
	try
	{
		throw std::runtime_error( _TXT( "no weighting function feature parameters allowed"));
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding weighting feature to '%s' weighting: %s"), THIS_METHOD_NAME, *m_errorhnd);
}

void WeightingFunctionContextScalar::setVariableValue( const std::string& name_, double value)
{
	m_func->setVariableValue( name_, value);
}

void WeightingFunctionContextScalar::fillParameter( double* param) const
{
	param[ 0] = m_nofCollectionDocuments;
	std::size_t pi = NOF_IMPLICIT_ARGUMENTS;
	std::vector<Index>::const_iterator mi = m_metadatahnd.begin(), me = m_metadatahnd.end();
	for (; mi != me; ++mi)
	{
		param[ pi++] = m_metadata->getValue( *mi).tofloat();
	}
}

const std::vector<WeightedField>& WeightingFunctionContextScalar::call( const Index& docno)
{
	try
	{
		m_lastResult.resize( 0);
		if (!m_metadatahnd.empty())
		{
			m_metadata->skipDoc( docno);
		}
		unsigned int nofParam = m_metadatahnd.size()+NOF_IMPLICIT_ARGUMENTS;
		double param[ MaxNofParameter+NOF_IMPLICIT_ARGUMENTS];
		fillParameter( param);

		double ww = m_func->call( param, nofParam);
		m_lastResult.resize( 1);
		m_lastResult[ 0].setWeight( ww);
		return m_lastResult;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error calling weighting function '%s': %s"), THIS_METHOD_NAME, *m_errorhnd, m_lastResult);
}

void WeightingFunctionInstanceScalar::addStringParameter( const std::string& name_, const std::string& value)
{
	try
	{
		if (m_func.get()) m_func.reset();

		if (strus::caseInsensitiveEquals( name_, "function"))
		{
			if (!m_expression.empty())
			{
				throw std::runtime_error( _TXT( "expression defined twice"));
			}
			m_expression = value;
		}
		else if (strus::caseInsensitiveEquals( name_, "metadata"))
		{
			if (m_metadataar.size() > WeightingFunctionContextScalar::MaxNofParameter)
			{
				throw strus::runtime_error(_TXT( "too many metadata parameter defined: %u"), (unsigned int)m_metadataar.size());
			}
			m_metadataar.push_back( value);
		}
		else
		{
			throw strus::runtime_error(_TXT( "unknown string type parameter: %s"), name_.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding weighting function string parameter to '%s' weighting: %s"), THIS_METHOD_NAME, *m_errorhnd);
}

void WeightingFunctionInstanceScalar::addNumericParameter( const std::string& name_, const NumericVariant& value)
{
	try
	{
		if (strus::caseInsensitiveEquals( name_, "metadata"))
		{
			throw strus::runtime_error(_TXT( "parameter '%s' not expected as numeric"), name_.c_str());
		}
		else
		{
			m_paramar.push_back( std::pair<std::string,double>( name_, value));
			if (m_func.get()) m_func->setDefaultVariableValue( name_, value);
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding weighting function string parameter to '%s' weighting: %s"), THIS_METHOD_NAME, *m_errorhnd);
}

void WeightingFunctionInstanceScalar::initFunction() const
{
	if (!m_parser)
	{
		m_parser = m_queryproc->getScalarFunctionParser("");
		if (!m_parser)
		{
			throw strus::runtime_error(_TXT("failed parse scalar function %s"), m_errorhnd->fetchError());
		}
	}
	// ... if the function has not yet be created, we create it (mutable m_func)
	std::vector<std::string> arguments;
	arguments.push_back( "N");
	arguments.insert( arguments.end(), m_metadataar.begin(), m_metadataar.end());
	m_func.reset( m_parser->createFunction( m_expression, arguments));
	if (!m_func.get())
	{
		throw strus::runtime_error(_TXT("failed parse scalar function %s"), m_errorhnd->fetchError());
	}
	std::vector<std::pair<std::string,double> >::const_iterator
		pi = m_paramar.begin(), pe = m_paramar.end();
	for (; pi != pe; ++pi)
	{
		m_func->setDefaultVariableValue( pi->first, pi->second);
	}
}

std::vector<std::string> WeightingFunctionInstanceScalar::getVariables() const
{
	try
	{
		if (!m_func.get()) initFunction();

		return m_func->getVariables();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error getting variables of the '%s' weighting function: %s"), THIS_METHOD_NAME, *m_errorhnd, std::vector<std::string>());
}

WeightingFunctionContextInterface* WeightingFunctionInstanceScalar::createFunctionContext(
		const StorageClientInterface* storage_,
		const GlobalStatistics& stats) const
{
	try
	{
		strus::Reference<MetaDataReaderInterface> metadata( storage_->createMetaDataReader());
		if (!metadata.get()) throw strus::runtime_error( _TXT("failed to create meta data reader: %s"), m_errorhnd->fetchError());

		if (!m_func.get()) initFunction();

		std::vector<Index> metadatahnd;
		std::vector<std::string>::const_iterator mi = m_metadataar.begin(), me = m_metadataar.end();
		for (; mi != me; ++mi)
		{
			Index elemhnd = metadata->elementHandle( *mi);
			if (elemhnd < 0)
			{
				throw strus::runtime_error(_TXT("metadata element '%s' is not defined"), mi->c_str());
			}
			metadatahnd.push_back( elemhnd);
		}
		GlobalCounter nofdocs = stats.nofDocumentsInserted()>=0?stats.nofDocumentsInserted():(GlobalCounter)storage_->nofDocumentsInserted();
		return new WeightingFunctionContextScalar( m_func.get(), metadata.release(), metadatahnd, (double)nofdocs, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating weighting function '%s' execution context: %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}

StructView WeightingFunctionInstanceScalar::view() const
{
	try
	{
		StructView rt;
		if (m_func.get())
		{
			rt( "function", m_func->view());
		}
		std::vector<std::pair<std::string,double> >::const_iterator
			pi = m_paramar.begin(), pe = m_paramar.end();
		for (; pi != pe; ++pi)
		{
			rt( pi->first, pi->second);
		}
		if (!m_metadataar.empty())
		{
			rt( "metadata", m_metadataar);
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summarizer introspection view: %s"), THIS_METHOD_NAME, *m_errorhnd, std::string());
}


WeightingFunctionInstanceInterface* WeightingFunctionScalar::createInstance(
		const QueryProcessorInterface* processor) const
{
	try
	{
		return new WeightingFunctionInstanceScalar( processor, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of weighting function '%s': %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}

StructView WeightingFunctionScalar::view() const
{
	try
	{
		typedef FunctionDescription P;
		FunctionDescription rt( name(), _TXT("Calculate the document weight with a weighting scheme defined by a scalar function on metadata elements,constants and variables as arguments."));
		rt( P::String, "function", _TXT( "defines the expression of the scalar function to execute"), "");
		rt( P::Metadata, "metadata", _TXT("defines a meta data element as additional parameter of the function besides N (collection size). The parameter is addressed by the name of the metadata element in the expression"));
		rt( P::Numeric, "[a-z]+", _TXT("defines a variable value to be substituted in the scalar function expression"));
		return std::move(rt);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating weighting function description for '%s': %s"), THIS_METHOD_NAME, *m_errorhnd, FunctionDescription());
}


