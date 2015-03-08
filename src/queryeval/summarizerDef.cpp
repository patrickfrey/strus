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
#include "summarizerDef.hpp"
#include "strus/summarizerFunctionInterface.hpp"
#include "private/utils.hpp"
#include "queryEval.hpp"

using namespace strus;

static std::size_t countArguments( char const** arg)
{
	if (!arg) return 0;
	std::size_t aidx = 0;
	for (; arg[aidx]; ++aidx){}
	return aidx;
}

SummarizerDef::SummarizerDef(
		const std::string& resultAttribute_, const SummarizerFunctionInterface* function_, const std::string& functionName_, const SummarizerConfig& config)
	:m_function(function_)
	,m_functionName(functionName_)
	,m_resultAttribute(resultAttribute_)
{
	m_numericParameters.resize( countArguments( m_function->numericParameterNames()));
	m_textualParameters.resize( countArguments( m_function->textualParameterNames()));

	std::map<std::string,ArithmeticVariant>::const_iterator
		pi = config.numericParameters().begin(),
		pe = config.numericParameters().end();

	const char** arg = m_function->numericParameterNames();
	if (!arg && pi != pe)
	{
		throw std::runtime_error( std::string( "no numeric arguments expected for summarizer '") + m_functionName + "'");
	}
	for (; pi != pe; ++pi)
	{
		std::size_t aidx = 0;
		for (; arg[aidx]; ++aidx)
		{
			if (utils::caseInsensitiveEquals( pi->first, arg[aidx]))
			{
				break;
			}
		}
		if (!arg[aidx])
		{
			throw std::runtime_error(std::string( "unknown numeric argument name '") + pi->first + "' for summarizer '" + m_functionName + "'");
		}
		m_numericParameters[aidx] = pi->second;
	}

	std::map<std::string,std::string>::const_iterator
		ti = config.textualParameters().begin(),
		te = config.textualParameters().end();

	arg = m_function->textualParameterNames();
	if (!arg && ti != te)
	{
		throw std::runtime_error( std::string( "no textual arguments expected for summarizer '") + m_functionName + "'");
	}
	for (; ti != te; ++ti)
	{
		std::size_t aidx = 0;
		for (; arg[aidx]; ++aidx)
		{
			if (utils::caseInsensitiveEquals( ti->first, arg[aidx]))
			{
				break;
			}
		}
		if (!arg[aidx])
		{
			throw std::runtime_error(std::string( "unknown textual argument name '") + ti->first + "' for summarizer '" + m_functionName + "'");
		}
		m_textualParameters[aidx] = ti->second;
	}

	std::map<std::string,std::string>::const_iterator
		fi = config.featureParameters().begin(),
		fe = config.featureParameters().end();
	arg = m_function->featureParameterClassNames();
	if (!arg && fi != fe)
	{
		throw std::runtime_error( std::string( "no feature arguments expected for summarizer function '") + m_functionName + "'");
	}
	for (; fi != fe; ++fi)
	{
		std::size_t aidx = 0;
		for (; arg[aidx]; ++aidx)
		{
			if (utils::caseInsensitiveEquals( fi->first, arg[aidx]))
			{
				break;
			}
		}
		if (!arg[aidx])
		{
			throw std::runtime_error(std::string( "unknown feature argument name '") + fi->first + "' for summarizer function '" + m_functionName + "'");
		}
		m_featureParameters.push_back( Feature( aidx, fi->second));
	}
}

