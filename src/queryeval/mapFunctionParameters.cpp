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
#include "mapFunctionParameters.hpp"
#include <boost/algorithm/string.hpp>

using namespace strus;

#error DEPRECATED 

std::vector<ArithmeticVariant>
	strus::mapFunctionParameters(
		const char** paramNames,
		const KeyMap<ArithmeticVariant>& paramDefs)
{
	char const** pi = paramNames;
	char const** pe = pi;

	std::vector<ArithmeticVariant> rt;
	for (; *pe; ++pe)
	{
		rt.push_back( ArithmeticVariant());
	}
	KeyMap<ArithmeticVariant>::const_iterator
		ai = paramDefs.begin(), ae = paramDefs.end();
	for (; ai != ae; ++ai)
	{
		pi = paramNames;
		for (int pidx=0; pi != pe; ++pi,++pidx)
		{
			if (boost::algorithm::iequals( ai->first, *pi))
			{
				rt[ pidx] = ai->second;
				break;
			}
		}
		if (pi == pe)
		{
			throw std::runtime_error( std::string("unknown argument name '") + ai->first + "'");
		}
	}
	return rt;
}

