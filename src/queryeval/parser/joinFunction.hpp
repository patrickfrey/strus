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
#ifndef _STRUS_QUERY_PARSER_JOIN_FUNCTION_HPP_INCLUDED
#define _STRUS_QUERY_PARSER_JOIN_FUNCTION_HPP_INCLUDED
#include "keyMap.hpp"
#include <string>
#include <vector>

namespace strus {
namespace parser {

class JoinFunction
{
public:
	class Argument
	{
	public:
		enum Type {Variable, SubExpression};
		
		Type type() const		{return m_type;}
		unsigned int idx() const	{return m_idx;}

		Argument( Type type_, unsigned int idx_)
			:m_type(type_),m_idx(idx_){}
		Argument( const Argument& o)
			:m_type(o.m_type),m_idx(o.m_idx){}

	private:
		Type m_type;
		unsigned int m_idx;
	};

	JoinFunction()
		:m_range(0){}
	JoinFunction( const JoinFunction& o)
		:m_name(o.m_name)
		,m_range(o.m_range)
		,m_args(o.m_args){}


	static unsigned int parse( char const*& src, std::vector<JoinFunction>& functions);

	const std::string& name() const			{return m_name;}
	int range() const				{return m_range;}
	const std::vector<Argument>& args() const	{return m_args;}

private:
	std::string m_name;			///< function name
	int m_range;				///< range
	std::vector<Argument> m_args;		///< function argument list
};

}}//namespace
#endif

