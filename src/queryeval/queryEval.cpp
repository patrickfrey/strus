
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
#include "strus/queryProcessorInterface.hpp"
#include "parser/lexems.hpp"
#include "parser/selectorSet.hpp"
#include "iteratorReference.hpp"
#include "accumulatorReference.hpp"
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <set>

#define STRUS_LOWLEVEL_DEBUG

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



void QueryEval::parseJoinOperationDef( char const*& src)
{
	int result = m_setnamemap.get( parse_IDENTIFIER( src));
	if (!isAlpha( *src) || !isEqual( parse_IDENTIFIER( src), "FOREACH"))
	{
		throw std::runtime_error("FOREACH expected after INTO and the destination set identifier of the join");
	}
	int selector = SelectorExpression::parse( src, m_selectors, m_setnamemap);
	if (!isAlpha( *src) || !isEqual( parse_IDENTIFIER( src), "DO"))
	{
		throw std::runtime_error( "DO expected after FOREACH and the selector expression");
	}
	int function = JoinFunction::parse( src, m_functions);
	if (!isSemiColon(*src))
	{
		throw std::runtime_error( "semicolon expected after a feature join operation definition");
	}
	parse_OPERATOR( src);
	m_operations.push_back( JoinOperation( result, function, selector));
}

void QueryEval::parseAccumulatorDef( char const*& src)
{
	if (m_accumulateOperation.defined())
	{
		throw std::runtime_error("duplicate definition of accumulator in query evaluation program");
	}
	m_accumulateOperation.parse( src, m_setnamemap);
	if (!isSemiColon(*src))
	{
		throw std::runtime_error( "missing semicolon ';' after EVAL expression");
	}
	parse_OPERATOR( src);
}

void QueryEval::parseTermDef( char const*& src)
{
	if (isAlpha(*src))
	{
		std::string termset = parse_IDENTIFIER( src);
		std::string termvalue;
		std::string termtype;

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
			throw std::runtime_error( "term value (string,identifier,number) after the feature group identifier");
		}
		if (!isColon( *src))
		{
			throw std::runtime_error( "colon (':') expected after term value");
		}
		parse_OPERATOR(src);
		if (!isAlpha( *src))
		{
			throw std::runtime_error( "term type identifier expected after colon and term value");
		}
		termtype = parse_IDENTIFIER( src);
		if (!isSemiColon( *src))
		{
			throw std::runtime_error( "semicolon expected after a feature declaration in the query");
		}
		parse_OPERATOR( src);
		m_predefinedTerms.push_back( Query::Term( termset, termtype, termvalue));
	}
	else
	{
		throw std::runtime_error( "feature set identifier expected as start of a term declaration in the query");
	}
}

QueryEval::QueryEval( const std::string& source)
{
	char const* src = source.c_str();
	enum StatementKeyword {e_INTO, e_EVAL, e_TERM};
	std::string id;

	skipSpaces( src);
	try
	{
		while (*src)
		{
			switch ((StatementKeyword)parse_KEYWORD( src, 3, "INTO", "EVAL", "TERM"))
			{
				case e_TERM:
					parseTermDef( src);
					break;
				case e_INTO:
					parseJoinOperationDef( src);
					break;
				case e_EVAL:
					parseAccumulatorDef( src);
					break;
			}
		}
	}
	catch (const std::runtime_error& e)
	{
		throw std::runtime_error(
			std::string( "error in query evaluation program ")
			+ errorPosition( source.c_str(), src)
			+ ":" + e.what());
	}
}


class QueryStruct
{
public:
	explicit QueryStruct( const StringIndexMap* setnamemap_)
		:m_setnamemap(setnamemap_){}

	void pushFeature( int termset, const IteratorReference& itr)
	{
		if (termset <= 0) throw std::runtime_error( "internal: term set identifier out of range");
		while ((std::size_t)termset > m_iteratorSets.size())
		{
			m_iteratorSets.push_back( std::vector< IteratorReference >());
		}
		m_iteratorSets[ termset-1].push_back( itr);
		if ((m_setSizeMap[ termset] += 1) > (int)QueryEval::MaxSizeFeatureSet)
		{
			throw std::runtime_error( std::string( "query term set '") + m_setnamemap->name( termset) + "' is getting too complex");
		}
	}

	const IteratorInterface* getFeature( int setIndex, std::size_t elemIndex)
	{
		const std::vector<IteratorReference>& iset = getFeatureSet( setIndex);
		if (iset.size() <= elemIndex)
		{
			throw std::runtime_error( std::string( "referencing feature '") + m_setnamemap->name( setIndex) + "' not defined yet completely (features have to be defined completely before referencing them in the program)");
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
			throw std::runtime_error( std::string( "referencing feature '") + m_setnamemap->name( setIndex) + "' not defined yet (features have to be defined before referencing them in the program)");
		}
		return m_iteratorSets[ setIndex-1];
	}

	const std::map<int,int> setSizeMap() const
	{
		return m_setSizeMap;
	}

	int setSize( int setIndex) const
	{
		std::map<int,int>::const_iterator gi = m_setSizeMap.find( setIndex);
		return (gi == m_setSizeMap.end())?0:gi->second;
	}

private:
	std::vector< std::vector<IteratorReference> > m_iteratorSets;
	const StringIndexMap* m_setnamemap;
	std::map<int,int> m_setSizeMap;
};


void QueryEval::print( std::ostream& out) const
{
	std::vector<Query::Term>::const_iterator ti = predefinedTerms().begin(), te = predefinedTerms().end();
	for (; ti != te; ++ti)
	{
		out << "TERM " << ti->set << ": " << ti->type << " '" << ti->value << "';" << std::endl;
	}
	std::vector<parser::JoinOperation>::const_iterator ji = m_operations.begin(), je = m_operations.end();
	for (; ji != je; ++ji)
	{
		out << "INTO " << m_setnamemap.name( ji->result())
			<< " FOREACH ";
		SelectorExpression::print( out, ji->selector(), m_selectors, m_setnamemap);

		out << " DO ";
		JoinFunction::print( out, ji->function(), m_functions);
		out << ";" << std::endl;
	}
	if (m_accumulateOperation.defined())
	{
		out << "EVAL ";
		m_accumulateOperation.print( out, m_setnamemap);
		out << ";" << std::endl;
	}
}

std::vector<WeightedDocument>
	QueryEval::getRankedDocumentList(
			AccumulatorInterface& accu,
			std::size_t maxNofRanks) const
{
	typedef std::multiset<WeightedDocument,WeightedDocument::CompareSmaller> Ranker;

	std::vector<WeightedDocument> rt;
	Ranker ranker;
	std::size_t ranks = 0;
	std::size_t maxNofRanksInspected = (maxNofRanks * 10) + 100;
	std::size_t nofRanksInspected = 0;

	Index docno = 0;
	unsigned int state = 0;
	unsigned int prev_state = 0;
	double weight = 0.0;

	while (accu.nextRank( docno, state, weight))
	{
		ranker.insert( WeightedDocument( docno, weight));
		if (ranks >= maxNofRanks)
		{
			ranker.erase( ranker.begin());
		}
		else
		{
			++ranks;
		}
		nofRanksInspected += 1;
		if (state > prev_state && nofRanksInspected > maxNofRanksInspected)
		{
			break;
		}
		prev_state = state;
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
		const Query& query_,
		std::size_t maxNofRanks) const
{
	QueryStruct query( &m_setnamemap);

#ifdef STRUS_LOWLEVEL_DEBUG
	std::cout << "Start evaluating query:" << std::endl;
	print( std::cout);
	std::cout << std::endl;
#endif
	//[1] Create the initial feature sets:
	{
		// Process the query features (explicit join operations)
		typedef std::pair<std::string, IteratorReference> FeatDef;
		std::vector<FeatDef> feats;

		std::vector<Query::Term>::const_iterator ti = query_.termar().begin(), te = query_.termar().end();
		std::vector<Query::JoinOp>::const_iterator ji = query_.joinar().begin(), je = query_.joinar().end();

		for (std::size_t tidx=0; ti != te; ++ti,++tidx)
		{
			feats.push_back( FeatDef( ti->set, processor.createTermIterator( ti->type, ti->value)));

			for (; ji != je && ji->termcnt == tidx; ++ji)
			{
				enum {MaxNofJoinopArguments=256};
				if (ji->nofArgs > MaxNofJoinopArguments || ji->nofArgs > feats.size())
				{
					throw std::runtime_error( "number of arguments of explicit join in query out of range");
				}
				const IteratorInterface* joinargs[ MaxNofJoinopArguments];
				std::size_t ii=0;
				for (;ii < ji->nofArgs; ++ii)
				{
					joinargs[ ji->nofArgs-ii-1] = feats[ feats.size()-ii-1].second.get();
				}
				IteratorReference res(
					processor.createJoinIterator(
						ji->opname, ji->range,
						ji->nofArgs, joinargs));

				feats.resize( feats.size() - ji->nofArgs);
				feats.push_back( FeatDef( ji->set, res));
			}
		}

		// Add add the processed query features to the initial feature set:
		std::vector<FeatDef>::const_iterator fi = feats.begin(), fe = feats.end();
		for (; fi != fe; ++fi)
		{
			StringIndexMap::const_iterator si = m_setnamemap.find( fi->first);
			if (si == m_setnamemap.end())
			{
				throw std::runtime_error( std::string( "term set identifier '") + fi->first + "' not used in this query program");
			}
			query.pushFeature( si->second, fi->second);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "Create query feature [" << si->second << "] " << fi->first << " :" << query.setSize(si->second) << std::endl;
#endif
		}

		// Add predefined terms to the initial feature set:
		std::vector<Query::Term>::const_iterator pi = m_predefinedTerms.begin(), pe = m_predefinedTerms.end();
		for (; pi != pe; ++pi)
		{
			StringIndexMap::const_iterator si = m_setnamemap.find( pi->set);
			if (si != m_setnamemap.end())
			{
				query.pushFeature( si->second, processor.createTermIterator( pi->type, pi->value));
#ifdef STRUS_LOWLEVEL_DEBUG
				std::cout << "create predefined feature [" << si->second << "] " << pi->set << " :" << query.setSize(si->second) << std::endl;
#endif
			}
		}
	}

	//[2] Iterate on all join operations and create the combined features:
	std::vector<parser::JoinOperation>::const_iterator ji = m_operations.begin(), je = m_operations.end();
	for (; ji != je; ++ji)
	{
		const parser::JoinFunction& function = functions()[ ji->function()-1];
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "calculate join ";
		function.print( std::cout, ji->function(), functions());
		std::cout << " of ";
		SelectorExpression::print( std::cout, ji->selector(), selectors(), m_setnamemap);
		std::cout << std::endl;
#endif
		SelectorSetR selset(
			SelectorSet::calculate(
				ji->selector(), selectors(), query.setSizeMap()));

		if (selset.get())
		{
			const std::vector<Selector>& selar = selset->ar();
			std::size_t ri = 0, re = selar.size(), ro = selset->rowsize();
			enum {MaxNofSelectorColumns=256};
			if (ro > MaxNofSelectorColumns)
			{
				throw std::runtime_error("query too complex (number of rows in selection has more than 256 elements");
			}
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "create feature " << m_setnamemap.name( ji->result()) << " from " << selset->tostring() << std::endl;
#endif
			const IteratorInterface* joinargs[ MaxNofSelectorColumns];
			for (; ri < re; ri += ro)
			{
				std::size_t ci = 0, ce = ro;
				for (; ci < ce; ++ci)
				{
					const Selector& sel = selar[ ri+ci];
					joinargs[ ci] = query.getFeature( sel.setIndex, sel.elemIndex);
				}
				IteratorInterface* opres =
					processor.createJoinIterator(
						function.name(), function.range(), ro, joinargs);
				query.pushFeature( ji->result(), opres);
			}
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "feature " << m_setnamemap.name( ji->result()) << " has set size " << query.setSize(ji->result()) << std::endl;
#endif
		}
	}
	//[3] Get the result accumulator and evaluate the results
	if (m_accumulateOperation.defined())
	{
		AccumulatorReference accumulator( processor.createAccumulator( m_accumulateOperation.name()));
	
		std::vector<WeightingFunction>::const_iterator
			gi = m_accumulateOperation.args().begin(),
			ge = m_accumulateOperation.args().end();
	
		for (std::size_t gidx=0; gi != ge; ++gi,++gidx)
		{
			const std::vector<IteratorReference>& feats = query.getFeatureSet( gi->setIndex());
			std::vector<IteratorReference>::const_iterator ai = feats.begin(), ae = feats.end();
			for (std::size_t aidx=0; ai != ae; ai++,aidx++)
			{
				if (ai->get())
				{
					accumulator->addRanker( gi->factor(), gi->function(), gi->params(), **ai);
				}
			}
		}
		std::vector<int>::const_iterator
			fi = m_accumulateOperation.featureSelectionSets().begin(),
			fe = m_accumulateOperation.featureSelectionSets().end();

		for (; fi != fe; ++fi)
		{
			const IteratorInterface* far[ MaxSizeFeatureSet];

			const std::vector<IteratorReference>& feats = query.getFeatureSet( *fi);
			if (feats.size() > MaxSizeFeatureSet)
			{
				throw std::runtime_error( "number of features in selection set is too big");
			}
			std::vector<IteratorReference>::const_iterator ai = feats.begin(), ae = feats.end();
			std::size_t aidx = 0;
			for (; ai != ae; ai++)
			{
				if (ai->get())
				{
					far[ aidx++] = ai->get();
				}
			}
			IteratorReference selection(
				processor.createJoinIterator( "union", 0, aidx, far));
			//... PF:HACK: Require 'union' to exist and do an efficient set union operation
	
			accumulator->addSelector( *selection);
		}
		return getRankedDocumentList( *accumulator, maxNofRanks);
	}
	else
	{
		throw std::runtime_error("no accumulator defined (EVAL), cannot evaluate ranked document list");
	}
}

