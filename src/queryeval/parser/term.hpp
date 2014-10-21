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
#ifndef _STRUS_QUERY_PARSER_TERM_HPP_INCLUDED
#define _STRUS_QUERY_PARSER_TERM_HPP_INCLUDED
#include "keyMap.hpp"
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

#error DEPRECATED

namespace strus {
namespace parser {

/// \brief Term definition in the query
class Term
{
public:
	Term()
		:m_resultset(0){}
	Term( const Term& o)
		:m_resultset(o.m_resultset),m_type(o.m_type),m_value(o.m_value){}
	Term( unsigned int resultset_, const std::string& type_, const std::string& value_)
		:m_resultset(resultset_),m_type(type_),m_value(value_){}

	unsigned int resultset() const				{return m_resultset;}
	const std::string& type() const				{return m_type;}
	const std::string& value() const			{return m_value;}

private:
	unsigned int m_resultset;				///< set index of the term occurencies
	std::string m_type;					///< term type string
	std::string m_value;					///< term value string
};

}}//namespace
#endif
