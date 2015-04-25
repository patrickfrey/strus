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
#include "private/utils.hpp"
#include "private/internationalization.hpp"

using namespace strus;

DLL_PUBLIC void strus::SummarizerConfig::defineNumericParameter( const std::string& name_, const ArithmeticVariant& value_)
{
	std::string name = utils::tolower( name_);
	if (m_numericParameters.find( name) != m_numericParameters.end())
	{
		throw strus::runtime_error( _TXT( "duplicate definition of summarizer parameter '%s'"), name_.c_str());
	}
	m_numericParameters[ name] = value_;
}

DLL_PUBLIC void strus::SummarizerConfig::defineStringParameter( const std::string& name_, const std::string& value_)
{
	std::string name = utils::tolower( name_);
	if (m_stringParameters.find( name) != m_stringParameters.end())
	{
		throw strus::runtime_error( _TXT( "duplicate definition of summarizer parameter '%s'"), name_.c_str());
	}
	m_stringParameters[ name] = value_;
}

DLL_PUBLIC void strus::SummarizerConfig::addFeatureParameter( const std::string& name_, const std::string& set_)
{
	std::string name = utils::tolower( name_);
	m_featureParameters.push_back( std::pair<std::string,std::string>(name,set_));
}


