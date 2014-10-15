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
#include <boost/shared_ptr.hpp>

namespace strus {
namespace parser {

/// \brief Defines a join operation of term occurrence sets
class JoinOperation
{
public:

	JoinOperation(
			unsigned int resultset_,
			const std::string& name_,
			int range_,
			const SelectorSetR& selectorset_)
		:m_resultset(resultset_)
		,m_name(name_)
		,m_range(range_)
		,m_selectorset(selectorset_){}

	JoinOperation( const JoinOperation& o)
		:m_resultset(o.m_resultset)
		,m_name(o.m_name)
		,m_range(o.m_range)
		,m_selectorset(o.m_selectorset){}

public:
	unsigned int resultset() const			{return m_resultset;}
	std::string name() const			{return m_name;}
	int range() const				{return m_range;}
	const SelectorSetR& selectorset() const		{return m_selectorset;}

private:
	unsigned int m_resultset;	///< join operation result set index
	std::string m_name;		///< name of operation
	int m_range;			///< range for operations defined in a range (position difference)
	SelectorSetR m_selectorset;	///< iterator reference sequences
};

}}//namespace
#endif
