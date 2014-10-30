
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
#include "parser/accumulateOperation.hpp"
#include "parser/lexems.hpp"
#include <stdexcept>

using namespace strus;
using namespace strus::parser;

void AccumulateOperation::parse( char const*& src, StringIndexMap& setnamemap)
{
	if (isAlpha( *src))
	{
		m_name = parse_IDENTIFIER( src);

		if (isOpenOvalBracket( *src))
		{
			parse_OPERATOR(src);
			m_args = WeightingFunction::parseExpression( src, setnamemap);
		}
		else
		{
			throw std::runtime_error( "oval brackets expected after accumulate function name");
		}
		if (!isAlpha( *src) || !isEqual( parse_IDENTIFIER( src), "WITH"))
		{
			throw std::runtime_error("WITH expected with the feature set selection list to determine what should be evaluated");
		}
		for (;;)
		{
			if (!isAlpha( *src))
			{
				throw std::runtime_error( "expected comma ',' separated list of identifiers defining the features to select after WITH");
			}
			m_featureSelectionSets.push_back(
				setnamemap.get( parse_IDENTIFIER( src)));

			if (!isComma( *src))
			{
				break;
			}
			++src;
		}
	}
	else
	{
		throw std::runtime_error( "name of accumulator function expected after EVAL");
	}
}


void AccumulateOperation::print( std::ostream& out, const StringIndexMap& setnamemap) const
{
	out << name();
	WeightingFunction::printExpression( out, args(), setnamemap);
	std::vector<int>::const_iterator si = m_featureSelectionSets.begin(), se = m_featureSelectionSets.end();
	out << " WITH ";
	for (int sidx=0; si != se; ++si,++sidx)
	{
		if (sidx) out << ", ";
		out << setnamemap.name(*si);
	}
}


