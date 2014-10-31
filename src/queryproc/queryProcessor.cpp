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
#include "queryProcessor.hpp"
#include "iterator/iteratorPred.hpp"
#include "iterator/iteratorSucc.hpp"
#include "iterator/iteratorIntersect.hpp"
#include "iterator/iteratorUnion.hpp"
#include "iterator/iteratorDifference.hpp"
#include "iterator/iteratorStructWithin.hpp"
#include "iterator/iteratorStructSequence.hpp"
#include "iteratorReference.hpp"
#include "weighting/estimatedNumberOfMatchesMap.hpp"
#include <string>
#include <vector>
#include <stdexcept>
#include <set>
#include <limits>

using namespace strus;

QueryProcessor::QueryProcessor( StorageInterface* storage_)
	:m_storage(storage_)
	,m_nofMatchesMap( new EstimatedNumberOfMatchesMap( storage_))
{}

static bool isEqual( const std::string& id, const char* idstr)
{
	char const* si = id.c_str();
	char const* di = idstr;
	for (; *si && *di && ((*si|32) == (*di|32)); ++si,++di){}
	return !*si && !*di;
}

IteratorInterface*
	QueryProcessor::createTermIterator( 
			const std::string& type,
			const std::string& value) const
{
	return m_storage->createTermOccurrenceIterator( type, value);
}

IteratorInterface*
	QueryProcessor::createJoinIterator(
		const std::string& name,
		int range,
		std::size_t nofargs,
		const IteratorInterface** args) const
{
	if (isEqual( name, "pred"))
	{
		if (range != 0) throw std::runtime_error( std::string( "no range argument expected for '") + name + "'");
		if (nofargs == 0) throw std::runtime_error( std::string( "too few arguments for '") + name + "'");
		if (nofargs >= 2) throw std::runtime_error( std::string( "too many arguments for '") + name + "'");

		return new IteratorPred( args[0]);
	}
	else if (isEqual( name, "succ"))
	{
		if (range != 0) throw std::runtime_error( std::string( "no range argument expected for '") + name + "'");
		if (nofargs == 0) throw std::runtime_error( std::string( "too few arguments for '") + name + "'");
		if (nofargs >= 2) throw std::runtime_error( std::string( "too many arguments for '") + name + "'");

		return new IteratorSucc( args[0]);
	}
	else if (isEqual( name, "union"))
	{
		if (range != 0) throw std::runtime_error( std::string( "no range argument expected for '") + name + "'");
		if (nofargs == 0) throw std::runtime_error( std::string( "too few arguments for '") + name + "'");

		return new IteratorUnion( nofargs, args);
	}
	else if (isEqual( name, "intersect"))
	{
		if (range != 0) throw std::runtime_error( std::string( "no range argument expected for '") + name + "'");
		if (nofargs == 0) throw std::runtime_error( std::string( "too few arguments for '") + name + "'");

		return new IteratorIntersect( nofargs, args);
	}
	else if (isEqual( name, "diff"))
	{
		if (range != 0) throw std::runtime_error( std::string( "no range argument expected for '") + name + "'");
		if (nofargs < 2) throw std::runtime_error( std::string( "too few arguments for '") + name + "'");
		if (nofargs > 2) throw std::runtime_error( std::string( "too many arguments for '") + name + "'");

		if (!args[0]) return 0;
		if (!args[1]) return args[0]->copy();

		return new IteratorDifference( args[0], args[1]);
	}
	else if (isEqual( name, "sequence_struct"))
	{
		if (nofargs < 2) throw std::runtime_error( std::string( "too few arguments for '") + name + "'");

		return new IteratorStructSequence( range, nofargs-1, args+1, args[0]);
	}
	else if (isEqual( name, "sequence"))
	{
		if (nofargs < 1) throw std::runtime_error( std::string( "too few arguments for '") + name + "'");
		
		return new IteratorStructSequence( range, nofargs, args);
	}
	else if (isEqual( name, "within_struct"))
	{
		if (nofargs < 2) throw std::runtime_error( std::string( "too few arguments for '") + name + "'");

		return new IteratorStructWithin( range, nofargs-1, args+1, args[0]);
	}
	else if (isEqual( name, "within"))
	{
		if (nofargs < 1) throw std::runtime_error( std::string( "too few arguments for '") + name + "'");

		return new IteratorStructWithin( range, nofargs, args);
	}
	else
	{
		throw std::runtime_error( std::string( "unknown term occurrence join operator '") + name + "'");
	}
}


double QueryProcessor::getEstimatedIdf(
			IteratorInterface& itr)
{
	return m_nofMatchesMap->get( itr);
}


WeightingFunctionInterface*
	QueryProcessor::createWeightingFunction(
			const std::string& name,
			const std::vector<float>& parameter) const
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


