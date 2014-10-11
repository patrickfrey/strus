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
#ifndef _STRUS_QUERY_TUPLE_GENERATOR_HPP_INCLUDED
#define _STRUS_QUERY_TUPLE_GENERATOR_HPP_INCLUDED
#include <string>
#include <vector>

namespace strus
{

/// \brief Class to generate set of tuples out of sets by standard combinatoric operations
class TupleGenerator
{
public:
	enum Mode
	{
		Product,		///< Product of sets: creates a set of element sequences from a squence of sets by generating any combination of the members in their order
		Ascending,		///< Creates all ascending permutations of a certain length 
		Permutation		///< Creates all permutations of a certain length 
	};

	explicit TupleGenerator( Mode mode_);

	void defineColumn( std::size_t max_);

	bool empty()					{return m_empty;}

	std::size_t column( std::size_t idx) const	{return m_columns[ idx].value;}
	std::size_t columns() const			{return m_columns.size();}

	bool next();

private:
	bool incrementIndex( std::size_t& idx, const std::size_t& maximum);

	struct Column
	{
		Column()
			:value(0),maximum(0){}
		Column( const Column& o)
			:value(o.value),maximum(o.maximum){}
		Column( std::size_t v, std::size_t m)
			:value(v),maximum(m){}

		std::size_t value;
		std::size_t maximum;
	};

private:
	bool m_empty;				///< true if element generation finished (no element defined)
	Mode m_mode;				///< type of tuple
	std::vector<Column> m_columns;		///< column values of the tuple
	std::vector<bool> m_occupied;		///< value defined by an element of the permutation
};

}//namespace
#endif

