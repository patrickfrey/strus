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
#include "summarizerConfig.hpp"
#include "strus/summarizerFunctionInterface.hpp"
#include "queryEval.hpp"
#include <boost/algorithm/string.hpp>

using namespace strus;

static std::size_t countArguments( char const** arg)
{
	if (!arg) return 0;
	std::size_t aidx = 0;
	for (; arg[aidx]; ++aidx){}
	return aidx;
}

SummarizerConfig::SummarizerConfig( QueryEval* qeval_, const SummarizerFunctionInterface* function_, const std::string& functionName_)
		:m_qeval(qeval_)
		,m_function(function_)
		,m_functionName(functionName_)
{
	m_numericParameters.resize( countArguments( m_function->numericParameterNames()));
	m_textualParameters.resize( countArguments( m_function->textualParameterNames()));
}

void SummarizerConfig::defineNumericParameter( const std::string& name_, const ArithmeticVariant& value_)
{
	char const** arg = m_function->numericParameterNames();
	if (!arg)
	{
		throw std::runtime_error( std::string( "no numeric arguments expected for summarizer function '") + m_functionName + "'");
	}
	std::size_t aidx = 0;
	for (; arg[aidx]; ++aidx)
	{
		if (boost::algorithm::iequals( name_, arg[aidx]))
		{
			if (m_numericParameters[ aidx].defined())
			{
				throw std::runtime_error(std::string( "duplicate definition of argument '") + name_ + "' for summarizer function '" + m_functionName + "'");
			}
			m_numericParameters[ aidx] = value_;
			break;
		}
	}
	if (!arg[aidx])
	{
		throw std::runtime_error(std::string( "unknown summarizer argument name '") + name_ + "' for summarizer function '" + m_functionName + "'");
	}
}

void SummarizerConfig::defineTextualParameter( const std::string& name_, const std::string& value_)
{
	char const** arg = m_function->textualParameterNames();
	if (!arg)
	{
		throw std::runtime_error( std::string( "no textual arguments expected for summarizer function '") + m_functionName + "'");
	}
	std::size_t aidx = 0;
	for (; arg[aidx]; ++aidx)
	{
		if (boost::algorithm::iequals( name_, arg[aidx]))
		{
			if (!m_textualParameters[ aidx].empty())
			{
				throw std::runtime_error(std::string( "duplicate definition of summarizer argument '") + name_ + "' for summarizer function '" + m_functionName + "'");
			}
			m_textualParameters[ aidx] = value_;
			break;
		}
	}
	if (!arg[aidx])
	{
		throw std::runtime_error(std::string( "unknown summarizer argument name '") + name_ + "' for summarizer function '" + m_functionName + "'");
	}
}

void SummarizerConfig::defineFeatureParameter( const std::string& class_, const std::string& set_)
{
	char const** arg = m_function->featureParameterClassNames();
	if (!arg)
	{
		throw std::runtime_error( std::string( "no feature arguments expected for summarizer function '") + m_functionName + "'");
	}
	std::size_t aidx = 0;
	for (; arg[aidx]; ++aidx)
	{
		if (boost::algorithm::iequals( class_, arg[aidx]))
		{
			break;
		}
	}
	if (!arg[aidx])
	{
		throw std::runtime_error(std::string( "unknown feature argument name '") + class_ + "' for summarizer function '" + m_functionName + "'");
	}
	m_featureParameters.push_back( Feature( aidx, set_));
}

void SummarizerConfig::done()
{
	m_qeval->defineSummarizer( *this);
}

