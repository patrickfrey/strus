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
#ifndef _STRUS_QUERY_PARSER_ACCUMULATE_OPERATION_HPP_INCLUDED
#define _STRUS_QUERY_PARSER_ACCUMULATE_OPERATION_HPP_INCLUDED
#include "keyMap.hpp"
#include "weightingFunction.hpp"
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

namespace strus {
namespace parser {

/// \brief Defines an accumulate operation
class AccumulateOperation
{
public:

	AccumulateOperation(){}
	AccumulateOperation(
			const std::string& name_,
			const std::vector<WeightingFunction>& args_)
		:m_name(name_)
		,m_args(args_){}

	AccumulateOperation( const AccumulateOperation& o)
		:m_name(o.m_name)
		,m_args(o.m_args)
	{}

	bool defined() const					{return !m_name.empty();}

	std::string name() const				{return m_name;}
	const std::vector<WeightingFunction>& args() const	{return m_args;}

	void parse( char const*& src, StringIndexMap& setnamemap);
	void print( std::ostream& out, const StringIndexMap& setnamemap) const;

private:
	std::string m_name;						///< name of operation
	std::vector<WeightingFunction> m_args;				///< list of set references with weight
};

}}//namespace
#endif

