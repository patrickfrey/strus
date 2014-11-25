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
#ifndef _STRUS_QUERY_PARSER_JOIN_OPERATION_HPP_INCLUDED
#define _STRUS_QUERY_PARSER_JOIN_OPERATION_HPP_INCLUDED
#include "keyMap.hpp"
#include "parser/selector.hpp"
#include <string>
#include <vector>

namespace strus {
namespace parser {

/// \brief Defines a join operation of term occurrence sets
class JoinOperation
{
public:
	JoinOperation(
			int result_,
			int function_,
			int selector_)
		:m_result(result_)
		,m_function(function_)
		,m_selector(selector_){}

	JoinOperation( const JoinOperation& o)
		:m_result(o.m_result)
		,m_function(o.m_function)
		,m_selector(o.m_selector){}

public:
	int result() const		{return m_result;}
	int function() const		{return m_function;}
	int selector() const		{return m_selector;}

private:
	int m_result;			///< join operation result set index
	int m_function;			///< index of function expression
	int m_selector;			///< set of argument sequences
};

}}//namespace
#endif

