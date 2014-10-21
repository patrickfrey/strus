
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
#include "queryEval.hpp"
#include "parser/lexems.hpp"
#include "parser/selectorSet.hpp"
#include "iteratorReference.hpp"
#include <stdexcept>

using namespace strus;
using namespace strus::parser;

static std::string errorPosition( const char* base, const char* itr)
{
	unsigned int line = 1;
	unsigned int col = 1;
	std::ostringstream msg;

	for (unsigned int ii=0,nn=itr-base; ii < nn; ++ii)
	{
		if (base[ii] == '\n')
		{
			col = 1;
			++line;
		}
		else
		{
			++col;
		}
	}
	msg << "at line " << line << " column " << col;
	return msg.str();
}

struct Expression
{
	Expression()
	{
		clear();
	}

	void clear()
	{
		result = 0;
		selector = 0;
		function = 0;
		duplicateflags = 0;
	}

	unsigned int duplicateflags;
	int result;
	int function;
	int selector;
};

QueryEval::QueryEval( std::string& source)
{
	char const* src = source.c_str();
	enum ExpressionKeyword {e_FOREACH, e_INTO, e_DO, e_EVAL};
	Expression expr;
	std::string id;

	skipSpaces( src);
	try
	{
		while (*src)
		{
			if (*src == ';')
			{
				if (expr.duplicateflags == 0)
				{
					throw std::runtime_error("empty expression (unexpected ';')");
				}
				if (!expr.function)
				{
					throw std::runtime_error("missing function (DO) in operation definition");
				}
				if (!expr.selector)
				{
					throw std::runtime_error("missing selector (FOREACH) in operation definition");
				}
				if (!expr.result)
				{
					throw std::runtime_error("missing result set (INTO) in operation definition");
				}
				m_operations.push_back( JoinOperation( expr.result, expr.function, expr.selector));
				expr.clear();
				expr.duplicateflags = 0;
			}
			switch ((ExpressionKeyword)parse_KEYWORD( expr.duplicateflags, src, 4, "FOREACH", "INTO", "DO", "EVAL"))
			{
				case e_FOREACH:
					expr.selector = SelectorExpression::parse( src, m_selectors, m_setnamemap);
					break;
				case e_INTO:
					expr.result = m_setnamemap.get( parse_IDENTIFIER( src));
					break;
				case e_DO:
					expr.function = JoinFunction::parse( src, m_expressions);
					break;
				case e_EVAL:
					if (!m_accumulateOperation.name().empty())
					{
						throw std::runtime_error("duplicate definition of accumulator in query evaluation program");
					}
					if (expr.duplicateflags)
					{
						throw std::runtime_error( "unterminated query expression (missing ';')");
					}
					m_accumulateOperation.parse( src, m_setnamemap);
					skipSpaces( src);
					if (*src != ';')
					{
						throw std::runtime_error( "missing semicolon ';' after EVAL expression");
					}
					parse_OPERATOR( src);
					break;
			}
		}
		if (expr.duplicateflags)
		{
			throw std::runtime_error( "unterminated query expression (missing ';') at end of query evaluation program");
		}
	}
	catch (const std::runtime_error& e)
	{
		throw std::runtime_error(
			std::string( "error in query evaluation program ")
			+ errorPosition( qry.c_str(), src)
			+ ":" + e.what());
	}
}


class QueryStruct
{
	explicit QueryStruct( const StringIndexMap* setnamemap_)
		:m_setnamemap(setnamemap_){}

	void pushFeature( int termset, const IteratorReference& itr)
	{
		if (termset <= 0) throw std::runtime_error( "internal: term set identifier out of range");
		while (termset > m_iteratorSets.size())
		{
			m_iteratorSets.push_back( std::vector< IteratorReference >());
		}
		m_iteratorSets[ termset-1].push_back( itr);
		m_setSizeMap[ termset] += 1;
	}

	const IteratorInterface* getFeature( int setIndex, std::size_t elemIndex)
	{
		const std::vector<IteratorReference>& iset = getFeatureSet( setIndex);
		if (iset.size() <= elemIndex)
		{
			throw std::runtime_error( std::string( "referencing feature '") + getFeatureName(setIndex) + "' not defined yet completely (features have to be defined completely before referencing them in the program)");
		}
		return iset[ elemIndex].get();
	}

	const std::vector<IteratorReference>& getFeatureSet( int setIndex)
	{
		if (setIndex <= 0)
		{
			throw std::runtime_error( "internal: feature address out of range");
		}
		if ((std::size_t)(unsigned int)setIndex > m_iteratorSets.size())
		{
			throw std::runtime_error( std::string( "referencing feature '") + getFeatureName(setIndex) + "' not defined yet (features have to be defined before referencing them in the program)");
		}
		return m_iteratorSets[ setIndex];
	}

	const std::map<int,int> setSizeMap() const
	{
		return m_setSizeMap;
	}

	const std::string getFeatureName( int setIndex) const
	{
		StringIndexMap::const_iterator si = m_setnamemap.begin(), se = m_setnamemap.end();
		for (; si != se; ++si)
		{
			if (si->second == setIndex)
			{
				return si->first;
			}
		}
	}

private:
	std::vector< std::vector<IteratorReference> > m_iteratorSets;
	const StringIndexMap* m_setnamemap;
	std::map<int,int> m_setSizeMap;
};


std::vector<WeightedDocument>
	QueryEval::getRankedDocumentList(
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


std::vector<WeightedDocument>
	QueryEval::getRankedDocumentList(
		const QueryProcessorInterface& processor,
		const std::string& querystr,
		std::size_t maxNofRanks) const
{
	char const* src = querystr.c_str();
	std::string termset;
	std::string termtype;
	std::string termvalue;
	QueryStruct query( &m_setnamemap);

	skipSpaces( src);

	//[1] Parse query string and create initial feature sets:
	try
	{
		while (!*src)
		{
			if (isAlpha(*src))
			{
				termset = parse_IDENTIFIER( src);
				if (!isAlpha( *src))
				{
					throw std::runtime_error( "term type identifier expected after set identifier in the query");
				}
				termtype = parse_IDENTIFIER( src);
				if (isSemiColon( *src))
				{
					termvalue.clear();
				}
				else
				{
					if (isStringQuote( *src))
					{
						termvalue = parse_STRING( src);
					}
					else if (isAlpha( *src))
					{
						termvalue = parse_IDENTIFIER( src);
					}
					else
					{
						throw std::runtime_error( "term value (string,identifier,number) or semicolon expected after a term type in the query");
					}
					if (!isSemiColon( *src))
					{
						throw std::runtime_error( "semicolon expected after a term declaration in the query");
					}
				}
				StringIndexMap::const_iterator ti = m_setnamemap.find( termset);
				if (ti == m_setnamemap.end())
				{
					throw std::runtime_error( std::string( "term set identifier '") + termset + "' not used in this query program");
				}
				termsetidx = ti->second;
	
				query.pushFeature( termsetidx, processor.createIterator( termtype, termvalue));
			}
			else
			{
				throw std::runtime_error( "feature set identifier expected as start of a term declaration in the query");
			}
		}
	}
	catch (const std::runtime_error& err)
	{
		throw std::runtime_error(
			std::string( "error in query string ")
			+ errorPosition( querystr.c_str(), src)
			+ ":" + e.what());
	}

	//[2] Iterate on all join operations and create the combineed features:
	std::vector<parser::JoinOperation>::const_iterator ji = m_operations.begin(), je = m_operations.end();
	for (; ji != je; ++ji)
	{
		const parser::JoinFunction& function = expressions()[ ji->function()];
		const parser::SelectorExpression& selector = selectors()[ ji->selector()];
		int result = ji->result();

		SelectorSetR selset(
			SelectorSet::calculate(
				ji->selector(), selectors(), query.setSizeMap()));

		const std::vector<Selector>& selar = selset->ar();
		std::size_t ri = 0, re = selar.size(), ro = selset->rowsize();
		enum {MaxNofSelectorColumns=256};
		if (ro > MaxNofSelectorColumns)
		{
			throw std::runtime_error("query too complex (number of rows in selection has more than 256 elements");
		}
		const IteratorInterface* joinargs[ MaxNofSelectorColumns];
		for (; ri < re; ri += ro)
		{
			std::size_t ci = 0, ce = ro;
			for (; ci < ce; ++ci)
			{
				const Selector& sel = selar[ ri+ci];
				joinargs[ ci] = query.getFeature( set.setIndex, set.elemIndex);
			}
			IteratorInterface* opres =
				processor.createIterator(
					function.name(), function.range(), ro, joinargs);
			query.pushFeature( ji->result(), opres);
		}
	}

	//[3] Get the result accumulator and evaluate the results
	accumulator.reset( processor->createAccumulator( m_accumulateOperation.name()));

	std::vector<WeightingFunction>::const_iterator
		gi = m_accumulateOperation.args().begin(),
		ge = m_accumulateOperation.args().end();

	for (std::size_t gidx=0; gi != ge; ++gi,++gidx)
	{
		const std::vector<IteratorReference>& feats = query.getFeatureSet( gi->setIndex);
		std::vector<IteratorReference>::const_iterator ai = feats.begin(), ae = feats.end();
		for (std::size_t aidx=0; ai != ae; ai++,aidx++)
		{
			accumulator->add( gi->factor, gi->function, gi->params, **ai);
		}
	}
	return getRankedDocumentList( *accumulator, maxNofRanks);
}

