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

namespace strus {
namespace parser {

/// \brief Defines an accumulate operation
class AccumulateOperation
{
public:

	AccumulateOperation(){}

	AccumulateOperation(
			const std::vector<WeightingFunction>& args_,
			const std::vector<int>& featureSelectionSets_)
		:m_args(args_)
		,m_featureSelectionSets(featureSelectionSets_){}

	AccumulateOperation( const AccumulateOperation& o)
		:m_args(o.m_args)
		,m_featureSelectionSets(o.m_featureSelectionSets)
	{}

	bool defined() const					{return m_args.size();}

	const std::vector<WeightingFunction>& args() const	{return m_args;}
	const std::vector<int>& featureSelectionSets() const	{return m_featureSelectionSets;}

	void parse( char const*& src, StringIndexMap& setnamemap);
	void print( std::ostream& out, const StringIndexMap& setnamemap) const;

private:
	std::vector<WeightingFunction> m_args;		///< list of set references with weight
	std::vector<int> m_featureSelectionSets;	///< list of feature sets by index to be selected as matches for candidates of documents to be ranked
};

}}//namespace
#endif

