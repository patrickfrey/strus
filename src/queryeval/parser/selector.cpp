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
#include "parser/selector.hpp"
#include "parser/lexems.hpp"
#include <boost/scoped_array.hpp>
#include <set>

using namespace strus;
using namespace strus::parser;

void SelectorSet::pushRow( const Selector* row)
{
	for (std::size_t ii=0; ii<m_rowsize; ++ii)
	{
		m_ar.push_back( row[ii]);
	}
}

void SelectorSet::pushRow( std::vector<Selector>::const_iterator row)
{
	for (std::size_t ii=0; ii<m_rowsize; ++ii,++row)
	{
		m_ar.push_back( *row);
	}
}

void SelectorSet::append( const SelectorSet& o)
{
	if (m_rowsize != o.m_rowsize)
	{
		throw std::runtime_error("joining incompatible sets (with different number of rows)");
	}
	else
	{
		m_ar.insert( m_ar.end(), o.m_ar.begin(), o.m_ar.end());
	}
}

SelectorSetR SelectorSet::calculate(
		TupleGenerator::Mode genmode,
		const std::vector<SelectorSetR>& selset)
{
	SelectorSetR rt;
	std::size_t rowsize = 0;
	std::vector<SelectorSetR>::const_iterator si = selset.begin(), se = selset.end();
	for (; si != se; ++si)
	{
		if (!si->get() || (*si)->ar().size() == 0) return rt;
		rowsize += (*si)->rowsize();
	}
	if (rowsize == 0) return rt;

	rt.reset( new SelectorSet( rowsize));
	boost::scoped_array<Selector> curRef( new Selector[ rowsize]);
	Selector* cur = curRef.get();

	TupleGenerator gen( genmode);
	for (si = selset.begin(); si != se; ++si)
	{
		gen.defineColumn( (*si)->nofrows());
	}

	if (!gen.empty()) do
	{
		// Build product row:
		std::size_t cntidx = 0;
		std::size_t rowidx = 0;
		for (si = selset.begin(); si != se; ++si,++cntidx)
		{
			std::size_t ri = (*si)->rowsize() * gen.column( cntidx);
			for (std::size_t ci=0; ci<(*si)->rowsize(); ++ci)
			{
				cur[ rowidx + ci] = (*si)->ar()[ ri + ci];
			}
			rowidx += (*si)->rowsize();
		}
		if (rowidx != rowsize) throw std::logic_error("query parser assertion failed: rowidx != rowsize");
		rt->pushRow( cur);
	}
	while (gen.next());
	return rt;
}

SelectorSetR SelectorSet::parseAtomic( char const*& src, strus::KeyMap<SetDimDescription>& setmap)
{
	std::string setname( IDENTIFIER( src));
	strus::KeyMap<SetDimDescription>::iterator si = setmap.find( setname);
	if (si == setmap.end())
	{
		return SelectorSetR();
	}
	else
	{
		si->second.referenced = true;

		SelectorSetR elemreflist( new SelectorSet( 1));
		std::size_t ei=0, ee=si->second.nofElements;
		for (; ei!=ee; ++ee)
		{
			Selector row( si->second.id, ei);
			elemreflist->pushRow( &row);
		}
		return elemreflist;
	}
}


SelectorSetR SelectorSet::parseExpression( char const*& src, strus::KeyMap<SetDimDescription>& setmap)
{
	skipSpaces( src);
	enum SelectorFunction
	{
		ProductFunc,
		AscendingFunc,
		PermutationFunc,
		SequenceFunc,
		DistinctFunc,
		JoinFunc
	};
	SelectorFunction function = ProductFunc;
	unsigned int dim = 0;
	std::vector<SelectorSetR> selset;

	for (;;)
	{
		if (isAlpha( *src))
		{
			bool isFunction = false;
			char const* src_bk = src;
			std::string functionName( IDENTIFIER( src));
	
			if (isEqual( functionName, "product"))
			{
				isFunction = true;
				function = ProductFunc;
				if (isDigit( *src))
				{
					dim = UNSIGNED( src);
				}
				else
				{
					dim = 0;
				}
			}
			else if (isEqual( functionName, "asc"))
			{
				isFunction = true;
				function = AscendingFunc;
				if (isDigit( *src))
				{
					dim = UNSIGNED( src);
				}
				else
				{
					dim = 0;
				}
			}
			else if (isEqual( functionName, "seq"))
			{
				isFunction = true;
				function = SequenceFunc;
				if (isDigit( *src))
				{
					dim = UNSIGNED( src);
				}
				else
				{
					dim = 0;
				}
			}
			else if (isEqual( functionName, "distinct"))
			{
				isFunction = true;
				function = DistinctFunc;
				if (isDigit( *src))
				{
					throw std::runtime_error( "unexpected dimension in distinct selector");
				}
				else
				{
					dim = 0;
				}
			}
			else if (isEqual( functionName, "permute"))
			{
				isFunction = true;
				function = PermutationFunc;
				if (isDigit( *src))
				{
					dim = UNSIGNED( src);
				}
				else
				{
					dim = 0;
				}
			}
			else if (isEqual( functionName, "join"))
			{
				isFunction = true;
				function = JoinFunc;
				if (isDigit( *src))
				{
					throw std::runtime_error( "unexpected dimension in join selector");
				}
				else
				{
					dim = 0;
				}
			}
			if (isFunction)
			{
				if (isColon( *src))
				{
					OPERATOR( src);
				}
				else
				{
					src = src_bk;
				}
			}
			else
			{
				if (isColon( *src))
				{
					throw std::runtime_error( std::string( "unknown function name '") + functionName + "'");
				}
				else
				{
					src = src_bk;
				}
			}
		}
		if (!*src)
		{
			throw std::runtime_error( "unexpected end of query in argument tuple set builder expression");
		}
		else if (isCloseSquareBracket(*src))
		{
			if (dim == 0)
			{
				dim = selset.size();
			}
			switch (function)
			{
				case ProductFunc:
				{
					if (selset.empty()) return SelectorSetR();
					if (dim < selset.size())
					{
						throw std::runtime_error("dimension of product selection is too small");
					}
					while (dim > selset.size())
					{
						selset.push_back( selset.back());
					}
					return SelectorSet::calculate( TupleGenerator::Product, selset);
				}
				case AscendingFunc:
				{
					if (selset.empty()) return SelectorSetR();
					while (dim > selset.size())
					{
						selset.push_back( selset.back());
					}
					if (dim < selset.size())
					{
						throw std::runtime_error("dimension of ascending selection is too small");
					}
					if (dim == selset.size())
					{
						return SelectorSet::calculate( TupleGenerator::Sequence, selset);
					}
					else
					{
						return SelectorSet::calculate( TupleGenerator::Ascending, selset);
					}
				}
				case SequenceFunc:
				{
					if (selset.empty()) return SelectorSetR();
					while (dim > selset.size())
					{
						selset.push_back( selset.back());
					}
					if (dim < selset.size())
					{
						throw std::runtime_error("dimension of sequence selection is too small");
					}
					return SelectorSet::calculate( TupleGenerator::Sequence, selset);
				}
				case PermutationFunc:
				{
					if (selset.empty()) return SelectorSetR();
					while (dim > selset.size())
					{
						selset.push_back( selset.back());
					}
					if (dim < selset.size())
					{
						throw std::runtime_error("dimension of permutation selection is too small");
					}
					return SelectorSet::calculate( TupleGenerator::Permutation, selset);
				}
				case DistinctFunc:
				{
					// ... select only tuples with distinct members
					if (selset.empty()) return SelectorSetR();
					SelectorSetR rt( new SelectorSet( selset.back()->rowsize()));
					std::vector<SelectorSetR>::const_iterator si = selset.begin(), se = selset.end();
					for (; si != se; ++si)
					{
						const std::vector<Selector>& ar = (*si)->ar();
						std::size_t ri=0, re=(*si)->nofrows(), ce = (*si)->rowsize();
						for (; ri < re; ri += ce)
						{
							std::set<Selector> dup;
							std::size_t ci=0;
							for (; ci < ce; ++ci)
							{
								dup.insert( ar[ri+ci]);
							}
							if (dup.size() == ce)
							{
								rt->pushRow( ar.begin() + ri);
							}
						}
					}
					return rt;
				}
				case JoinFunc:
				{
					if (selset.empty()) return SelectorSetR();
					SelectorSetR rt( new SelectorSet( selset.back()->rowsize()));
					std::vector<SelectorSetR>::const_iterator si = selset.begin(), se = selset.end();
					for (; si != se; ++si)
					{
						rt->append( **si);
					}
					return rt;
				}
			}
		}
		else if (isOpenSquareBracket(*src))
		{
			OPERATOR( src);
			selset.push_back( parseExpression( src, setmap));
		}
		else if (isAlpha(*src))
		{
			selset.push_back( parseAtomic( src, setmap));
		}
		else
		{
			throw std::runtime_error( "set identifier or set tuple builder expression in square brackets '[' ']' expected");
		}
	}
}


