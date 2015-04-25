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

SummarizerDef::SummarizerDef(
		const std::string& resultAttribute_, const SummarizerFunctionInterface* function_, const std::string& functionName_, const SummarizerConfig& config)
	:m_function(function_->createInstance())
	,m_functionName(functionName_)
	,m_resultAttribute(resultAttribute_)
	,m_featureParameters(config.featureParameters())
{
	std::map<std::string,ArithmeticVariant>::const_iterator
		pi = config.numericParameters().begin(),
		pe = config.numericParameters().end();
	for (; pi != pe; ++pi)
	{
		m_function->addNumericParameter( pi->first, pi->second);
	}

	std::map<std::string,std::string>::const_iterator
		ti = config.stringParameters().begin(),
		te = config.stringParameters().end();
	for (; ti != te; ++ti)
	{
		m_function->addStringParameter( ti->first, ti->second);
	}
}

