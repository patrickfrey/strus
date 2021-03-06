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
#include "parser/selectorSet.hpp"
#include "parser/lexems.hpp"
#include <boost/scoped_array.hpp>
#include <set>
#include <sstream>
#include <iostream>

using namespace strus;
using namespace strus::parser;

#undef STRUS_LOWLEVEL_DEBUG

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

SelectorSet* SelectorSet::calculateJoin(
		const LocalReferenceArray<SelectorSet>& argsets)
{
	LocalReference<SelectorSet> rt;
	LocalReferenceArray<SelectorSet>::const_iterator si = argsets.begin(), se = argsets.end();
	if (si == se) return 0;
	std::size_t rowsize = 0;
	for (; si != se; ++si)
	{
		if (si->rowsize() > 0)
		{
			if (rowsize == 0)
			{
				rowsize = si->rowsize();
			}
			else if (rowsize != si->rowsize())
			{
				throw std::runtime_error("number of rows in selector expression do not match in join");
			}
		}
	}
	if (rowsize == 0)
	{
		return 0;
	}
	rt.reset( new SelectorSet( rowsize));
	si = argsets.begin();
	for (; si != se; ++si)
	{
		if (si->rowsize())
		{
			std::vector<Selector>::const_iterator
				ai = si->ar().begin(), ae = si->ar().end();
			for (; ai != ae; ai += rowsize)
			{
				rt->pushRow( ai);
			}
		}
	}
	return rt.detach();
}

SelectorSet* SelectorSet::calculateTuple(
		TupleGenerator::Mode genmode,
		bool distinct,
		const LocalReferenceArray<SelectorSet>& argsets)
{
	std::size_t rowsize = 0;
	LocalReferenceArray<SelectorSet>::const_iterator
		si = argsets.begin(), se = argsets.end();
	for (; si != se; ++si)
	{
		if (si->ar().size() == 0) return 0;
		rowsize += si->rowsize();
	}
	if (rowsize == 0) return 0;

	LocalReference<SelectorSet> rt( new SelectorSet( rowsize));

	boost::scoped_array<Selector> curRef( new Selector[ rowsize]);
	Selector* cur = curRef.get();

	TupleGenerator gen( genmode);
	for (si = argsets.begin(); si != se; ++si)
	{
		gen.defineColumn( si->nofrows());
	}

	if (!gen.empty()) do
	{
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "Generated tuple element: " << gen.tostring() << std::endl;
#endif
		// Build product row:
		std::size_t cntidx = 0;
		std::size_t rowidx = 0;
		for (si = argsets.begin(); si != se; ++si,++cntidx)
		{
			std::size_t ri = si->rowsize() * gen.column( cntidx);
			for (std::size_t ci=0; ci<si->rowsize(); ++ci)
			{
				cur[ rowidx + ci] = si->ar()[ ri + ci];
			}
			rowidx += si->rowsize();
		}
		if (rowidx != rowsize) throw std::logic_error( "query parser assertion failed: rowidx != rowsize");
		if (distinct)
		{
			std::set<Selector> dupset;
			for (std::size_t ri=0; ri<rowsize; ++ri)
			{
				dupset.insert( cur[ri]);
			}
			if (dupset.size() == rowsize)
			{
				rt->pushRow( cur);
			}
		}
		else
		{
			rt->pushRow( cur);
		}
	}
	while (gen.next());
	return rt.detach();
}

SelectorSet* SelectorSet::calculate(
		int expressionidx,
		const std::vector<parser::SelectorExpression>& expressions,
		const std::map<int,int>& setSizeMap)
{
	LocalReference<SelectorSet> rt;
	if (expressionidx <= 0 || (std::size_t)expressionidx > expressions.size())
	{
		throw std::runtime_error( "expression index out of range");
	}
	const parser::SelectorExpression& expression = expressions[ expressionidx-1];

	// [1] Build the list of arguments:
	LocalReferenceArray<SelectorSet> argsets;
	std::vector<SelectorExpression::Argument>::const_iterator ai = expression.args().begin(), ae = expression.args().end();
	for (; ai != ae; ++ai)
	{
		switch (ai->type())
		{
			case SelectorExpression::Argument::SetReference:
			{
				argsets.push_back( new SelectorSet(1));
				SelectorSet& argset = argsets.back();
				std::map<int,int>::const_iterator mi = setSizeMap.find( ai->idx());
				if (mi != setSizeMap.end())
				{
					int ii=0, nn=mi->second;
					for (; ii<nn; ++ii)
					{
						Selector row( ai->idx(), ii);
#ifdef STRUS_LOWLEVEL_DEBUG
						std::cout << "Selector push set reference (" << ai->idx() << "," << ii << ")" << std::endl;
#endif
						argset.pushRow( &row);
					}
				}
				break;
			}
			case SelectorExpression::Argument::SubExpression:
			{
				SelectorSet* subresult
					= calculate( ai->idx(), expressions, setSizeMap);
				if (!subresult)
				{
					return 0;
				}
				argsets.push_back( subresult);
			}
		}
	}

	// [1] Calculate the selector operation:
	switch (expression.functionid())
	{
		case SelectorExpression::ProductFunc: 
			rt = calculateTuple( TupleGenerator::Product, false, argsets);
			break;
		case SelectorExpression::AscendingFunc:
			rt = calculateTuple( TupleGenerator::Ascending, false, argsets);
			break;
		case SelectorExpression::PermutationFunc:
			rt = calculateTuple( TupleGenerator::Permutation, false, argsets);
			break;
		case SelectorExpression::SequenceFunc:
			rt = calculateTuple( TupleGenerator::Sequence, false, argsets);
			break;
		case SelectorExpression::DistinctFunc:
			rt = calculateTuple( TupleGenerator::Product, true, argsets);
			break;
		case SelectorExpression::JoinFunc:
			rt = calculateJoin( argsets);
			break;
	}
#ifdef STRUS_LOWLEVEL_DEBUG
	std::cout << "Selector set calculation result: " << (rt.get()?rt->tostring():std::string("EMPTY")) << std::endl;
#endif
	return rt.detach();
}

std::string SelectorSet::tostring() const
{
	std::ostringstream rt;
	rt << "{";
	std::vector<Selector>::const_iterator si = m_ar.begin(), se = m_ar.end();
	for (unsigned int sidx=0; si != se; ++si,++sidx)
	{
		if (sidx == rowsize())
		{
			rt << "), (";
			sidx = 0;
		}
		else if (sidx)
		{
			rt << ", ";
		}
		else
		{
			rt << "(";
		}
		rt << "[" << si->setIndex << ":" << si->elemIndex << "]";
	}
	if (m_ar.size())
	{
		rt << ")";
	}
	rt << "}";
	return rt.str();
}

