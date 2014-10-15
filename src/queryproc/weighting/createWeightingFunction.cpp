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
#include "weighting/createWeightingFunction.hpp"
#include "weighting/weightingFrequency.hpp"
#include "weighting/weightingConstant.hpp"
#include "weighting/weightingStorageWeight.hpp"
#include "weighting/weightingBM25.hpp"
#include <stdexcept>

using namespace strus;

static bool isEqual( const std::string& id, const char* idstr)
{
	char const* si = id.c_str();
	char const* di = idstr;
	for (; *si && *di && ((*si|32) == (*di|32)); ++si,++di){}
	return !*si && !*di;
}

WeightingFunctionInterface* strus::createWeightingFunction(
		const StorageInterface* storage,
		const EstimatedNumberOfMatchesMapR& nofMatchesMap,
		const std::string& function,
		const std::vector<float>& parameter)
{
	if (isEqual( function, "weight"))
	{
		if (!parameter.empty()) throw std::runtime_error( std::string("unexpected scaling parameter for accumulator '") + function + "'");
		return new WeightingStorageWeight( storage, nofMatchesMap);
	}
	else if (isEqual( function, "td"))
	{
		if (!parameter.empty()) throw std::runtime_error( std::string("unexpected scaling parameter for accumulator '") + function + "'");
		return new WeightingConstant( 1.0, storage, nofMatchesMap);
	}
	else if (isEqual( function, "tf"))
	{
		if (!parameter.empty()) throw std::runtime_error( std::string("unexpected scaling parameter for accumulator '") + function + "'");
		return new WeightingFrequency( storage, nofMatchesMap);
	}
	else if (isEqual( function, "bm25"))
	{
		float b  = parameter.size() > 0 ? parameter[0]:0.75;
		float k1 = parameter.size() > 1 ? parameter[1]:1.5;
		float avgDocLength = parameter.size() > 2 ? parameter[2]:1000;

		return new WeightingBM25( storage, nofMatchesMap, k1, b, avgDocLength);
	}
	else
	{
		throw std::runtime_error( std::string( "unknown weighting function '") + function + "'");
	}
}



