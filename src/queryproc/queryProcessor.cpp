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
#include "queryproc/queryProcessor.hpp"
#include "queryproc/iterator/iteratorIntersect.hpp"
#include "queryproc/iterator/iteratorUnion.hpp"
#include "queryproc/iterator/iteratorCutInRange.hpp"
#include <stdexcept>
#include <set>

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

IteratorReference QueryProcessor::createIterator( 
			const std::string& type,
			const std::string& value)
{
	return IteratorReference( m_storage->createTermOccurrenceIterator( type, value));
}

IteratorReference QueryProcessor::createIterator(
			const std::string& name,
			const std::vector<std::string>& options,
			const std::vector<IteratorReference>& arg)
{
	if (isEqual( name, "union"))
	{
		if (arg.empty()) throw std::runtime_error( std::string( "too few arguments for '") + name + "'");
		if (arg.size() < 2) return arg[0];
		IteratorReference rt( new IteratorUnion( arg[0], arg[1]));
		std::size_t ii=2,ie=arg.size();
		for (; ii<ie; ++ie)
		{
			IteratorReference join( new IteratorUnion( rt, arg[ii]));
			rt = join;
		}
		return rt;
	}
	else if (isEqual( name, "intersect"))
	{
		if (arg.empty()) throw std::runtime_error( std::string( "too few arguments for '") + name + "'");
		if (arg.size() < 2) return arg[0];
		IteratorReference rt( new IteratorIntersect( arg[0], arg[1]));
		std::size_t ii=2,ie=arg.size();
		for (; ii<ie; ++ie)
		{
			IteratorReference join( new IteratorIntersect( rt, arg[ii]));
			rt = join;
		}
		return rt;
	}
	else if (isEqual( name, "cirange"))
	{
		if (arg.size() < 2) throw std::runtime_error( std::string( "too few arguments for '") + name + "'");
		if (arg.size() > 3) throw std::runtime_error( std::string( "too many arguments for '") + name + "'");
		IteratorReference cut( (arg.size() > 2)?arg[2]:IteratorReference());
		std::vector<std::string>::const_iterator oi = options.begin(), oe = options.end();
		bool withFirstElemCut = false;
		bool withLastElemCut = false;
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
		return IteratorReference( new IteratorCutInRange( arg[0], arg[1], cut, range, withFirstElemCut, withLastElemCut));
	}
	else
	{
		throw std::runtime_error( std::string( "unknown term occurrence join function '") + name + "'");
	}
}

AccumulatorReference QueryProcessor::createAccumulator(
			const std::string& name,
			const std::vector<double>& scale,
			const std::vector<double>& weights,
			const std::vector<AccumulatorReference>& arg)
{
	return AccumulatorReference();
}

AccumulatorReference QueryProcessor::createOccurrenceAccumulator(
			const std::string& /*name*/,
			const std::vector<IteratorReference>& /*arg*/)
{
	return AccumulatorReference();
}


std::vector<WeightedDocument> QueryProcessor::getRankedDocumentList(
			const AccumulatorReference& resultaccu,
			std::size_t maxNofRanks) const
{
	std::vector<WeightedDocument> rt;
	typedef std::multiset<WeightedDocument,WeightedDocument::CompareSmaller> Ranker;
	Ranker ranker;
	std::size_t ranks = 0;

	if (!resultaccu.get()) return rt;
	AccumulatorInterface& accu = *resultaccu.get();

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


