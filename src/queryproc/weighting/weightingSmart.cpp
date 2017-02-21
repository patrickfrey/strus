/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
///\file weightingSmart.cpp
///\brief Implementation of a weighting function defined as function on tf,df,N and some metadata references in a string
#include "weightingSmart.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/scalarFunctionInstanceInterface.hpp"
#include "strus/base/string_format.hpp"
#include "private/utils.hpp"
#include "private/errorUtils.hpp"
#include "private/internationalization.hpp"
#include <sstream>
#include <iostream>
#include <iomanip>

#define METHOD_NAME "SMART"

using namespace strus;

WeightingFunctionContextSmart::WeightingFunctionContextSmart(
		const ScalarFunctionInterface* func_,
		MetaDataReaderInterface* metadata_,
		const std::vector<Index>& metadatahnd_,
		double nofCollectionDocuments_,
		ErrorBufferInterface* errorhnd_)
	:m_func(func_->createInstance())
	,m_metadata(metadata_)
	,m_metadatahnd(metadatahnd_)
	,m_nofCollectionDocuments(nofCollectionDocuments_)
	,m_errorhnd(errorhnd_)
{
	if (!m_func.get())
	{
		throw strus::runtime_error(_TXT("failed to create weighting function instance"));
	}
}

void WeightingFunctionContextSmart::addWeightingFeature(
		const std::string& name_,
		PostingIteratorInterface* itr_,
		double weight_,
		const TermStatistics& stats_)
{
	try
	{
		if (utils::caseInsensitiveEquals( name_, "match"))
		{
			m_featar.push_back( Feature( itr_, (double)weight_, stats_));
		}
		else
		{
			throw strus::runtime_error( _TXT( "unknown '%s' weighting function feature parameter '%s'"), METHOD_NAME, name_.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding weighting feature to '%s' weighting: %s"), METHOD_NAME, *m_errorhnd);
}

void WeightingFunctionContextSmart::fillParameter( const Index& docno, double ff, double df, double* param) const
{
	param[ 0] = ff;
	param[ 1] = df;
	param[ 2] = m_nofCollectionDocuments;
	std::size_t pi = 3;
	std::vector<Index>::const_iterator mi = m_metadatahnd.begin(), me = m_metadatahnd.end();
	for (; mi != me; ++mi)
	{
		param[ pi++] = m_metadata->getValue( *mi).tofloat();
	}
}

double WeightingFunctionContextSmart::call( const Index& docno)
{
	if (!m_metadatahnd.empty())
	{
		m_metadata->skipDoc( docno);
	}
	double rt = 0.0;
	unsigned int nofParam = m_metadatahnd.size()+3;

	FeatureVector::iterator fi = m_featar.begin(), fe = m_featar.end();
	for (; fi != fe; ++fi)
	{
		double param[ MaxNofParameter+3];
		fillParameter( docno, docno == fi->skipDoc(docno) ? fi->ff() : 0, fi->df(), param);

		rt += m_func->call( param, nofParam);
	}
	return rt;
}

std::string WeightingFunctionContextSmart::debugCall( const Index& docno)
{
	std::ostringstream out;
	out << std::fixed << std::setprecision(8);
	out << string_format( _TXT( "calculate %s"), METHOD_NAME) << std::endl;

	if (!m_metadatahnd.empty())
	{
		m_metadata->skipDoc( docno);
	}
	double res = 0.0;
	unsigned int nofParam = m_metadatahnd.size()+3;

	FeatureVector::iterator fi = m_featar.begin(), fe = m_featar.end();
	for (unsigned int fidx=0; fi != fe; ++fi,++fidx)
	{
		double param[ MaxNofParameter+3];
		fillParameter( docno, docno == fi->skipDoc(docno) ? fi->ff() : 0, fi->df(), param);

		double ww = m_func->call( param, nofParam);
		res += ww;
		out << string_format( _TXT( "[%u] result=%f, ff=%u, df=%u, N=%u"),
					fidx, ww, (unsigned int)fi->ff(), (unsigned int)(fi->df()+0.5), (unsigned int)m_nofCollectionDocuments) << std::endl;			
		
		unsigned int pi=3, pe=nofParam;
		for (; pi != pe; ++pi)
		{
			out << string_format( _TXT( ", x%u=%f"), pi, param[pi+3]);
		}
		out << std::endl;
	}
	out << string_format( _TXT( "sum result=%f"), res) << std::endl;
	return out.str();
}

void WeightingFunctionInstanceSmart::addStringParameter( const std::string& name, const std::string& value)
{
	try
	{
		if (m_func.get()) m_func.reset();

		if (utils::caseInsensitiveEquals( name, "function"))
		{
			if (!m_expression.empty())
			{
				throw strus::runtime_error(_TXT( "expression defined twice"));
			}
			m_expression = value;
		}
		else if (utils::caseInsensitiveEquals( name, "metadata"))
		{
			if (m_metadataar.size() > WeightingFunctionContextSmart::MaxNofParameter)
			{
				throw strus::runtime_error(_TXT( "too many metadata parameter defined: %u"), m_metadataar.size());
			}
			m_metadataar.push_back( value);
		}
		else
		{
			throw strus::runtime_error(_TXT( "unknown string type parameter: %s"), name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding weighting function string parameter to '%s' weighting: %s"), METHOD_NAME, *m_errorhnd);
}

void WeightingFunctionInstanceSmart::addNumericParameter( const std::string& name, const NumericVariant& value)
{
	try
	{
		if (utils::caseInsensitiveEquals( name, "metadata"))
		{
			throw strus::runtime_error(_TXT( "parameter '%s' not expected as numeric"), name.c_str());
		}
		else
		{
			m_paramar.push_back( std::pair<std::string,double>( name, value));
			if (m_func.get()) m_func->setDefaultVariableValue( name, value);
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding weighting function string parameter to '%s' weighting: %s"), METHOD_NAME, *m_errorhnd);
}

WeightingFunctionContextInterface* WeightingFunctionInstanceSmart::createFunctionContext(
		const StorageClientInterface* storage_,
		MetaDataReaderInterface* metadata,
		const GlobalStatistics& stats) const
{
	try
	{
		if (!m_func.get())
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
			arguments.push_back( "ff");
			arguments.push_back( "df");
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
		return new WeightingFunctionContextSmart( m_func.get(), metadata, metadatahnd, (double)nofdocs, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating weighting function '%s' execution context: %s"), METHOD_NAME, *m_errorhnd, 0);
}

std::string WeightingFunctionInstanceSmart::tostring() const
{
	try
	{
		std::ostringstream msg;
		if (m_func.get())
		{
			msg << m_func->tostring() << std::endl;
			msg << "--" << std::endl;
		}
		std::vector<std::pair<std::string,double> >::const_iterator
			pi = m_paramar.begin(), pe = m_paramar.end();
		for (; pi != pe; ++pi)
		{
			msg << "\t" << pi->first << " = " << pi->second << std::endl;
		}
		std::vector<std::string>::const_iterator mi = m_metadataar.begin(), me = m_metadataar.end();
		for (; mi != me; ++mi)
		{
			msg << "\t" << *mi << " = <metadata>" << std::endl;
		}
		return msg.str();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error mapping weighting function '%s' to string: %s"), METHOD_NAME, *m_errorhnd, std::string());
}

WeightingFunctionInstanceInterface* WeightingFunctionSmart::createInstance(
		const QueryProcessorInterface* processor) const
{
	try
	{
		return new WeightingFunctionInstanceSmart( processor, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of weighting function '%s': %s"), METHOD_NAME, *m_errorhnd, 0);
}

FunctionDescription WeightingFunctionSmart::getDescription() const
{
	try
	{
		typedef FunctionDescription::Parameter P;
		FunctionDescription rt(_TXT("Calculate the document weight with a weighting scheme given by a scalar function defined as expression with ff (feature frequency), df (document frequency), N (total number of documents in the collection) and some specified metadata elements as arguments. The name of this method has been inspired by the traditional SMART weighting schemes in IR"));
		rt( P::Feature, "match", _TXT( "defines the query features to weight"), "");
		rt( P::String, "function", _TXT( "defines the expression of the scalar function to execute"), "");
		rt( P::Metadata, "metadata", _TXT("defines a meta data element as additional parameter of the function besides ff,df and N. The parameter is addressed by the name of the metadata element in the expression"));
		rt( P::Numeric, "[a-z]+", _TXT("defines a variable value to be substituted in the scalar function expression"));
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating weighting function description for '%s': %s"), METHOD_NAME, *m_errorhnd, FunctionDescription());
}


