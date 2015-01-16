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
#ifndef _STRUS_CASE_INSENSITIVE_STRING_MAP_HPP_INCLUDED
#define _STRUS_CASE_INSENSITIVE_STRING_MAP_HPP_INCLUDED
#include <string>
#include <map>
#include <boost/algorithm/string.hpp>

#error DEPRECATED

namespace strus
{

/// \class cistring
/// \brief Constructor for implementing implicit type reductions in key maps that store the key as upper case string.
struct cistring
	:public std::string
{
	cistring( const char* o)
		:std::string( boost::to_lower_copy( std::string(o))){}
	cistring( const std::string& o)
		:std::string( boost::to_lower_copy( o)){}
	cistring( const cistring& o)
		:std::string( o){}
	cistring(){}
};

/// \class cistringmap
/// \brief Map with case insensitive strings as keys
template <typename ValueType>
struct cistringmap
	:public std::map<cistring,ValueType>
{
	typedef std::map<cistring,ValueType> Parent;

	cistringmap(){}
	cistringmap( const cistringmap& o)
		:std::map<cistring,ValueType>(o){}
	cistringmap( const std::map<std::string,ValueType>& o)
	{
		std::copy( o.begin(), o.end(), std::inserter( *this, this->end()));
	}
};

}//namespace
#endif

