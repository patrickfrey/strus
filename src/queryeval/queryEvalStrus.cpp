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
#include "strus/queryeval/queryEvalStrus.hpp"
#include "strus/accumulatorInterface.hpp"
#include <cstring>

using namespace strus;

namespace {
class Program
{
public:
	Program()
		:m_minWeight(0){}

	Program( const std::string& query);

	Program( const Program& o)
		:m_strings(o.m_strings)
		,m_numberar(o.m_numberar)
		,m_iterar(o.m_iterar)
		,m_accuar(o.m_accuar)
		,m_rankar(o.m_rankar)
		,m_cmdlist(o.m_cmdlist)
		,m_minWeight(o.m_minWeight)
	{}

	AccumulatorInterfaceR accumulator( const StorageInterfaceR& storage) const;

	double minWeight()
	{
		return m_minWeight;
	}

private:
	struct Command
	{
		enum Type {StorageFetch,Union,Intersect,CutInRange,AccuFF,AccuSum};
		Type m_type;
		int m_op[3];

		Command()
		{
			std::memset( this, 0, sizeof(*this));
		}

		Command( const Command& o)
		{
			std::memcpy( this, &o, sizeof(*this));
		}

		Command( Type type_, int op1=0, int op2=0, int op3=0)
			:m_type(type_)
		{
			m_op[0] = op0;
			m_op[1] = op1;
			m_op[2] = op2;
		}
	};

private:
	std::string m_strings;
	std::vector<double> m_numberar;
	std::vector<IteratorInterfaceR> m_iterar;
	std::vector<AccumulatorInterfaceR> m_accuar;
	std::vector<RankerInterfaceR> m_rankar;
	std::vector<Command> m_cmdlist;
	double m_minWeight;
};

Program::Program( const std::string& query)
{
	
}

AccumulatorInterfaceR Program::accumulator( const StorageInterfaceR& storage) const
{
	
}

}//anonymous namespace

std::vector<WeightedDocument>
		QueryEvalStrus::evaluate(
			const StorageInterfaceR& storage,
			const RankerInterfaceR& ranker,
			const std::string& query,
			std::size_t maxNofRanks)
{
	Program prg( query);
	AccumulatorInterfaceR accu( prg.accumulator( storage));
	return ranker.calculate( accu, prg.minWeight(), maxNofRanks);
}



