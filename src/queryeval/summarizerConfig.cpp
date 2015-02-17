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
#include "strus/summarizerConfig.hpp"
#include "private/dll_tags.hpp"
#include <boost/algorithm/string.hpp>

using namespace strus;

DLL_PUBLIC void SummarizerConfig::defineNumericParameter( const std::string& name_, const ArithmeticVariant& value_)
{
	std::string name = boost::algorithm::to_lower_copy( name_);
	if (m_numericParameters.find( name) != m_numericParameters.end())
	{
		throw std::runtime_error( std::string("duplicate definition of summarizer parameter '") + name_ + "'");
	}
	m_numericParameters[ name] = value_;
}

DLL_PUBLIC void SummarizerConfig::defineTextualParameter( const std::string& name_, const std::string& value_)
{
	std::string name = boost::algorithm::to_lower_copy( name_);
	if (m_textualParameters.find( name) != m_textualParameters.end())
	{
		throw std::runtime_error( std::string("duplicate definition of summarizer parameter '") + name_ + "'");
	}
	m_textualParameters[ name] = value_;
}

DLL_PUBLIC void SummarizerConfig::defineFeatureParameter( const std::string& class_, const std::string& set_)
{
	std::string name = boost::algorithm::to_lower_copy( class_);
	if (m_featureParameters.find( name) != m_featureParameters.end())
	{
		throw std::runtime_error( std::string( "duplicate definition of summarizer feature parameter '") + class_ + "'");
	}
	m_featureParameters[ name] = set_;
}


