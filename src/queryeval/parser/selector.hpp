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
#ifndef _STRUS_QUERY_PARSER_SELECTOR_HPP_INCLUDED
#define _STRUS_QUERY_PARSER_SELECTOR_HPP_INCLUDED
#include "keyMap.hpp"
#include "parser/tupleGenerator.hpp"
#include "parser/setDimDescription.hpp"
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

namespace strus {
namespace parser {

struct Selector
{
	Selector( unsigned int setIndex_, unsigned int elemIndex_)
		:setIndex(setIndex_),elemIndex(elemIndex_){}
	Selector( const Selector& o)
		:setIndex(o.setIndex),elemIndex(o.elemIndex){}
	Selector()
		:setIndex(0),elemIndex(0){}

	unsigned int setIndex;
	unsigned int elemIndex;
};

class SelectorSet;
typedef boost::shared_ptr<SelectorSet> SelectorSetR;
		
class SelectorSet
{
public:
	SelectorSet( std::size_t rowsize_)
		:m_rowsize(rowsize_){}

	void pushRow( const Selector* row)		{for (std::size_t ii=0; ii<m_rowsize; ++ii) m_ar.push_back( row[ii]);}
	void append( const SelectorSet& o)		{if (m_rowsize != o.m_rowsize) throw std::runtime_error("joining incompatible sets (with different number of rows)"); else m_ar.insert( m_ar.end(), o.m_ar.begin(), o.m_ar.end());}

	const std::vector<Selector>& ar() const		{return m_ar;}
	std::size_t rowsize() const			{return m_rowsize;}
	std::size_t nofrows() const			{return m_rowsize ? (m_ar.size() / m_rowsize):0;}

	static SelectorSetR parseAtomic(
			char const*& src,
			strus::KeyMap<SetDimDescription>& setmap);
	static SelectorSetR parseExpression(
			char const*& src,
			strus::KeyMap<SetDimDescription>& setmap);

private:
	static SelectorSetR calculate(
			TupleGenerator::Mode genmode,
			const std::vector<SelectorSetR>& selset);

private:
	std::vector<Selector> m_ar;			///< iterator reference sequences
	std::size_t m_rowsize;				///< number of selector elements in a sequence
};



}}//namespace
#endif

