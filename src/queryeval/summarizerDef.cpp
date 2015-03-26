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
#include "private/internationalization.hpp"

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
		throw strus::runtime_error( _TXT( "no numeric arguments expected for summarizer '%s'"), m_functionName.c_str());
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
			throw strus::runtime_error( _TXT( "unknown numeric argument name '%s' for summarizer '%s'") ,pi->first.c_str(), m_functionName.c_str());
		}
		m_numericParameters[aidx] = pi->second;
	}

	std::map<std::string,std::string>::const_iterator
		ti = config.textualParameters().begin(),
		te = config.textualParameters().end();

	arg = m_function->textualParameterNames();
	if (!arg && ti != te)
	{
		throw strus::runtime_error( _TXT( "no textual arguments expected for summarizer '%s'"), m_functionName.c_str());
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
			throw strus::runtime_error( _TXT( "unknown textual argument name '%s' for summarizer '%s'"), ti->first.c_str(), m_functionName.c_str());
		}
		m_textualParameters[aidx] = ti->second;
	}

	std::map<std::string,std::string>::const_iterator
		fi = config.featureParameters().begin(),
		fe = config.featureParameters().end();
	arg = m_function->featureParameterClassNames();
	if (!arg && fi != fe)
	{
		throw strus::runtime_error( _TXT( "no feature arguments expected for summarizer function '%s'"), m_functionName.c_str());
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
			throw strus::runtime_error( _TXT( "unknown feature argument name '%s' for summarizer function '%s'"), fi->first.c_str(), m_functionName.c_str());
		}
		m_featureParameters.push_back( Feature( aidx, fi->second));
	}
}

