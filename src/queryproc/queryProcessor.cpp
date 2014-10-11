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
#include "iterator/iteratorIntersect.hpp"
#include "iterator/iteratorUnion.hpp"
#include "iterator/iteratorDifference.hpp"
#include "iterator/iteratorStructWithin.hpp"
#include "iterator/iteratorStructSequence.hpp"
#include "accumulator/accumulatorOperatorTemplate.hpp"
#include "accumulator/accumulatorOperators.hpp"
#include "iteratorReference.hpp"
#include "accumulatorReference.hpp"
#include <string>
#include <vector>
#include <stdexcept>
#include <set>
#include <limits>

using namespace strus;

static bool isEqual( const std::string& id, const char* idstr)
{
	char const* si = id.c_str();
	char const* di = idstr;
	for (; *si && *di && ((*si|32) == (*di|32)); ++si,++di){}
	return !*si && !*di;
}

IteratorInterface*
	QueryProcessor::createIterator( 
			const std::string& type,
			const std::string& value)
{
	return m_storage->createTermOccurrenceIterator( type, value);
}

IteratorInterface*
	QueryProcessor::createIterator(
		const std::string& name,
		int range,
		std::size_t nofargs,
		const IteratorInterface** args)
{
	if (isEqual( name, "union"))
	{
		if (range != 0) throw std::runtime_error( std::string( "no range argument expected for '") + name + "'");
		if (nofargs == 0) throw std::runtime_error( std::string( "too few arguments for '") + name + "'");

		IteratorInterface* rt = args[0]->copy();
		std::size_t ii=1;
		for (; ii<nofargs; ++ii)
		{
			rt = new IteratorUnion(
					IteratorReference(rt),
					IteratorReference(args[ii]->copy()));
		}
		return rt;
	}
	else if (isEqual( name, "intersect"))
	{
		if (range != 0) throw std::runtime_error( std::string( "no range argument expected for '") + name + "'");
		if (nofargs == 0) throw std::runtime_error( std::string( "too few arguments for '") + name + "'");

		IteratorInterface* rt = args[0]->copy();
		std::size_t ii=1;
		for (; ii<nofargs; ++ii)
		{
			rt = new IteratorIntersect(
					IteratorReference(rt),
					IteratorReference(args[ii]->copy()));
		}
		return rt;
	}
	else if (isEqual( name, "diff"))
	{
		if (range != 0) throw std::runtime_error( std::string( "no range argument expected for '") + name + "'");
		if (nofargs < 2) throw std::runtime_error( std::string( "too few arguments for '") + name + "'");
		if (nofargs > 2) throw std::runtime_error( std::string( "too many arguments for '") + name + "'");

		return new IteratorDifference( IteratorReference(args[0]->copy()), IteratorReference(args[1]->copy()));
	}
	else if (isEqual( name, "sequence_struct"))
	{
		if (nofargs < 2) throw std::runtime_error( std::string( "too few arguments for '") + name + "'");
		IteratorReference structDelimiter( args[0]->copy());
		std::vector<IteratorReference> seq;
		for (std::size_t ai=1; ai<nofargs; ++ai)
		{
			seq.push_back( IteratorReference( args[ai]->copy()));
		}
		return new IteratorStructSequence( seq, structDelimiter, range);
	}
	else if (isEqual( name, "sequence"))
	{
		if (nofargs < 1) throw std::runtime_error( std::string( "too few arguments for '") + name + "'");
		IteratorReference empty;
		std::vector<IteratorReference> seq;
		for (std::size_t ai=0; ai<nofargs; ++ai)
		{
			seq.push_back( IteratorReference( args[ai]->copy()));
		}
		return new IteratorStructSequence( seq, empty, range);
	}
	else if (isEqual( name, "within_struct"))
	{
		if (nofargs < 2) throw std::runtime_error( std::string( "too few arguments for '") + name + "'");
		IteratorReference structDelimiter( args[0]->copy());
		std::vector<IteratorReference> group;
		for (std::size_t ai=1; ai<nofargs; ++ai)
		{
			group.push_back( IteratorReference( args[ai]->copy()));
		}
		
		return new IteratorStructWithin( group, structDelimiter, range);
	}
	else if (isEqual( name, "within"))
	{
		if (nofargs < 1) throw std::runtime_error( std::string( "too few arguments for '") + name + "'");
		IteratorReference empty;
		std::vector<IteratorReference> group;
		for (std::size_t ai=0; ai<nofargs; ++ai)
		{
			group.push_back( IteratorReference( args[ai]->copy()));
		}
		return new IteratorStructWithin( group, empty, range);
	}
	else
	{
		throw std::runtime_error( std::string( "unknown term occurrence join operator '") + name + "'");
	}
}

AccumulatorInterface*
	QueryProcessor::createAccumulator(
		const std::string& /*name*/,
		const std::vector<double>& /*scale*/,
		std::size_t /*nofargs*/,
		const WeightedAccumulator* /*arg*/)
{
	return 0;
}

static std::vector<IteratorReference>
	copyIteratorArgs( std::size_t nofargs, const IteratorInterface** args)
{
	std::vector<IteratorReference> rt;
	std::size_t ii=0;
	for (; ii<nofargs; ++ii)
	{
		rt.push_back( IteratorReference( args[ii]->copy()));
	}
	return rt;
}

AccumulatorInterface*
	QueryProcessor::createOccurrenceAccumulator(
		const std::string& name,
		std::size_t nofargs,
		const IteratorInterface** args)
{
	if (isEqual( name, "weight"))
	{
		return new AccumulatorOperatorSum_weight( copyIteratorArgs( nofargs, args));
	}
	else if (isEqual( name, "td"))
	{
		return new AccumulatorOperatorSum_td( copyIteratorArgs( nofargs, args));
	}
	else if (isEqual( name, "tf"))
	{
		return new AccumulatorOperatorSum_tf( copyIteratorArgs( nofargs, args));
	}
	else if (isEqual( name, "td1"))
	{
		return new AccumulatorOperatorNormSum_td( copyIteratorArgs( nofargs, args));
	}
	else
	{
		throw std::runtime_error( std::string( "unknown occurrency accumulator '") + name + "'");
	}
}


std::vector<WeightedDocument>
	QueryProcessor::getRankedDocumentList(
			AccumulatorInterface& accu,
			std::size_t maxNofRanks) const
{
	typedef std::multiset<WeightedDocument,WeightedDocument::CompareSmaller> Ranker;

	std::vector<WeightedDocument> rt;
	Ranker ranker;
	std::size_t ranks = 0;

	Index docno = 0;
	int state = 0;
	double weigth = 0.0;

	while (accu.nextRank( docno, state, weigth))
	{
		ranker.insert( WeightedDocument( docno, weigth));
		if (ranks >= maxNofRanks)
		{
			ranker.erase( ranker.begin());
		}
		else
		{
			++ranks;
		}
	}
	Ranker::reverse_iterator ri=ranker.rbegin(),re=ranker.rend();
	for (; ri != re; ++ri)
	{
		rt.push_back( *ri);
	}
	return rt;
}


