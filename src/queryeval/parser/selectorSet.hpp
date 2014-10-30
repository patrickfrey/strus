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
#ifndef _STRUS_QUERY_PARSER_SELECTOR_SET_HPP_INCLUDED
#define _STRUS_QUERY_PARSER_SELECTOR_SET_HPP_INCLUDED
#include "parser/tupleGenerator.hpp"
#include "parser/selector.hpp"
#include "parser/selectorExpression.hpp"
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

namespace strus {
namespace parser {

class SelectorSet;
typedef boost::shared_ptr<SelectorSet> SelectorSetR;
		
class SelectorSet
{
public:
	SelectorSet( std::size_t rowsize_)
		:m_rowsize(rowsize_){}

	const std::vector<Selector>& ar() const		{return m_ar;}
	std::size_t rowsize() const			{return m_rowsize;}
	std::size_t nofrows() const			{return m_rowsize ? (m_ar.size() / m_rowsize):0;}

	static SelectorSetR calculate(
			int expressionidx,
			const std::vector<parser::SelectorExpression>& expressions,
			const std::map<int,int>& setSizeMap);

	std::string tostring() const;

private:
	void pushRow( const Selector* row);
	void pushRow( std::vector<Selector>::const_iterator row);
	void append( const SelectorSet& o);

	static SelectorSetR calculateJoin(
			const std::vector<SelectorSetR> argsets);

	static SelectorSetR calculateTuple(
			TupleGenerator::Mode genmode,
			bool distinct,
			const std::vector<SelectorSetR> argsets);

private:
	std::vector<Selector> m_ar;			///< iterator reference sequences
	std::size_t m_rowsize;				///< number of selector elements in a sequence
};

}}//namespace
#endif

