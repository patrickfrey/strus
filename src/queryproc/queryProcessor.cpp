/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013 Patrick Frey

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
#include "iterator/iteratorCutInRange.hpp"
#include "accumulator/accumulatorOperatorTemplate.hpp"
#include "accumulator/accumulatorOperators.hpp"
#include "iteratorReference.hpp"
#include "accumulatorReference.hpp"
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
static bool isDigit( char ch)
{
	return (ch >= '0' && ch <= '9');
}
static bool getUIntValue( unsigned int& result, const char* valuestr)
{
	unsigned int value = 0;
	unsigned int prev = 0;
	char const* vi = valuestr;
	if (!*vi) return false;
	for (; *vi >= '0' && *vi <= '9'; ++vi)
	{
		value = (value * 10) + (*vi - '0');
		if (prev > value) break;
		prev = value;
	}
	return !!*vi;
}

IteratorInterface* QueryProcessor::createIterator( 
			const std::string& type,
			const std::string& value)
{
	return m_storage->createTermOccurrenceIterator( type, value);
}

IteratorInterface*
	createIterator(
		const std::string& name,
		const std::vector<std::string>& options,
		std::size_t nofargs,
		const IteratorInterface** args)
{
	if (isEqual( name, "union"))
	{
		if (nofargs == 0) throw std::runtime_error( std::string( "too few arguments for '") + name + "'");
		if (nofargs < 2) return args[0]->copy();
		IteratorReference rt( new IteratorUnion( IteratorReference(args[0]->copy()), IteratorReference(args[1]->copy())));
		std::size_t ii=2;
		for (; ii<nofargs; ++ii)
		{
			IteratorReference join( new IteratorUnion( rt, IteratorReference(args[ii]->copy())));
			rt = join;
		}
		return rt->copy();
	}
	else if (isEqual( name, "intersect"))
	{
		if (nofargs == 0) throw std::runtime_error( std::string( "too few arguments for '") + name + "'");
		if (nofargs < 2) return args[0]->copy();
		IteratorReference rt( new IteratorIntersect( IteratorReference(args[0]->copy()), IteratorReference(args[1]->copy())));
		std::size_t ii=2;
		for (; ii<nofargs; ++ii)
		{
			IteratorReference join( new IteratorIntersect( rt, IteratorReference(args[ii]->copy())));
			rt = join;
		}
		return rt->copy();
	}
	else if (isEqual( name, "cirange"))
	{
		if (nofargs < 2) throw std::runtime_error( std::string( "too few arguments for '") + name + "'");
		if (nofargs > 3) throw std::runtime_error( std::string( "too many arguments for '") + name + "'");
		IteratorReference cut( (nofargs > 2)?IteratorReference(args[2]->copy()):IteratorReference());
		std::vector<std::string>::const_iterator oi = options.begin(), oe = options.end();
		bool withFirstElemCut = false;
		bool withLastElemCut = false;
		bool withFirstElemRange = false;
		unsigned int range = 1;

		for (; oi != oe; ++oi)
		{
			switch ((*oi)[0]|32)
			{
				case 'd':
					if (!getUIntValue( range, oi->c_str()+1))
					{
						if (oi->size() > 1 && !isDigit((*oi)[1]))
						{
							throw std::runtime_error( std::string( "unknown option '") + *oi + "' for join function 'cirange'");
						}
						else
						{
							throw std::runtime_error( std::string( "illegal argument for option 'd' for join function 'cirange'"));
						}
					}
					break;
				case 'a':
					withFirstElemRange = true;
					break;
				case 's':
					if (oi->size() > 1) throw std::runtime_error( std::string( "unknown option '") + *oi + "' for join function 'cirange'");
					withFirstElemCut = true;
					break;
				case 'e':
					if (oi->size() > 1) throw std::runtime_error( std::string( "unknown option '") + *oi + "' for join function 'cirange'");
					withLastElemCut = true;
					break;
				default:
					throw std::runtime_error( std::string( "unknown option '") + *oi + "' for join function 'cirange'");
			}
		}
		return new IteratorCutInRange(
				IteratorReference(args[0]->copy()),
				IteratorReference(args[1]->copy()),
				cut, range, 
				withFirstElemRange, withFirstElemCut, withLastElemCut);
	}
	else
	{
		throw std::runtime_error( std::string( "unknown term occurrence join function '") + name + "'");
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
	std::vector<WeightedDocument> rt;
	typedef std::multiset<WeightedDocument,WeightedDocument::CompareSmaller> Ranker;
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


