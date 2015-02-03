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
#include "query.hpp"
#include "queryEval.hpp"
#include "accumulator.hpp"
#include "ranker.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/constants.hpp"
#include "strus/attributeReaderInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/summarizerClosureInterface.hpp"
#include "strus/invAclIteratorInterface.hpp"
#include "strus/reference.hpp"
#include "keyMap.hpp"
#include <vector>
#include <string>
#include <utility>
#include <boost/algorithm/string.hpp>

using namespace strus;

///\brief Constructor
Query::Query( const QueryEval* queryEval_, const StorageInterface* storage_, const QueryProcessorInterface* processor_)
	:m_queryEval(queryEval_)
	,m_storage(storage_)
	,m_processor(processor_)
	,m_metaDataReader(storage_->createMetaDataReader())
{
	std::vector<QueryEval::TermDef>::const_iterator
		ti = m_queryEval->predefinedTerms().begin(),
		te = m_queryEval->predefinedTerms().end();
	for (; ti != te; ++ti)
	{
		pushTerm( ti->type, ti->value);
		defineFeature( ti->set, 1.0);
	}
}

Query::Query( const Query& o)
	:m_queryEval(o.m_queryEval)
	,m_processor(o.m_processor)
	,m_terms(o.m_terms)
	,m_expressions(o.m_expressions)
	,m_features(o.m_features)
	,m_stack(o.m_stack)
	,m_maxNofRanks(o.m_maxNofRanks)
	,m_minRank(o.m_minRank)
	,m_username(o.m_username)
{}


void Query::pushTerm( const std::string& type_, const std::string& value_)
{
	m_terms.push_back( Term( type_, value_));
	m_stack.push_back( nodeAddress( TermNode, m_terms.size()-1));
}

void Query::pushExpression( const std::string& opname_, std::size_t argc, int range_)
{
	if (argc > m_stack.size())
	{
		throw std::runtime_error( "illegal expression definition: size of expression bigger than stack size");
	}
	std::vector<NodeAddress> subnodes;
	std::vector<NodeAddress>::const_iterator si = m_stack.end() - argc, se = m_stack.end();
	for (; si != se; ++si)
	{
		subnodes.push_back( *si);
	}
	m_expressions.push_back( Expression( opname_, subnodes, range_));
	m_stack.resize( m_stack.size() - argc);
	m_stack.push_back( nodeAddress( ExpressionNode, m_expressions.size()-1));
}

void Query::defineFeature( const std::string& set_, float weight_)
{
	m_features.push_back( Feature( boost::algorithm::to_lower_copy(set_), m_stack.back(), weight_));
	m_stack.pop_back();
}

void Query::defineMetaDataRestriction(
		CompareOperator opr, const char* name,
		const ArithmeticVariant& operand, bool newGroup)
{
	Index hnd = m_metaDataReader->elementHandle( name);
	const char* typeName = m_metaDataReader->getType( hnd);
	m_metaDataRestrictions.push_back( MetaDataRestriction( typeName, opr, hnd, operand, newGroup));
}

void Query::defineFeatureRestriction( const std::string& set_)
{
	m_featureRestrictions.push_back( boost::algorithm::to_lower_copy( set_));
}

void Query::print( std::ostream& out)
{
	std::vector<Feature>::const_iterator fi = m_features.begin(), fe = m_features.end();
	for (; fi != fe; ++fi)
	{
		out << "feature '" << fi->set << "':" << std::endl;
		printNode( out, fi->node, 1);
		out << std::endl;
	}
}

void Query::printNode( std::ostream& out, NodeAddress adr, std::size_t indent)
{
	std::string indentstr( indent*2, ' ');
	switch (nodeType( adr))
	{
		case NullNode:
			out << indentstr << "NULL" << std::endl;
			break;
		case TermNode:
		{
			const Term& term = m_terms[ nodeIndex( adr)];
			out << "term " << term.type << " '" << term.value << "'" << std::endl;
			break;
		}
		case ExpressionNode:
		{
			const Expression& expr = m_expressions[ nodeIndex( adr)];
			out << expr.opname << " " << expr.range << ":" << std::endl;
			std::vector<NodeAddress>::const_iterator
				ni = expr.subnodes.begin(),
				ne = expr.subnodes.end();
			for (; ni != ne; ++ni)
			{
				printNode( out, *ni, indent+1);
			}
			break;
		}
	}
}

void Query::setMaxNofRanks( std::size_t maxNofRanks_)
{
	m_maxNofRanks = maxNofRanks_;
}

void Query::setMinRank( std::size_t minRank_)
{
	m_minRank = minRank_;
}

void Query::setUserName( const std::string& username_)
{
	m_username = username_;
}

PostingIteratorInterface* Query::createExpressionPostingIterator( const Expression& expr)
{
	const PostingJoinOperatorInterface*
		join = m_processor->getPostingJoinOperator( expr.opname);

	enum {MaxNofJoinopArguments=256};
	if (expr.subnodes.size() > MaxNofJoinopArguments)
	{
		throw std::runtime_error( "number of arguments of feature join expression in query out of range");
	}
	std::vector<Reference<PostingIteratorInterface> > joinargs;
	std::vector<NodeAddress>::const_iterator
		ni = expr.subnodes.begin(), ne = expr.subnodes.end();
	for (; ni != ne; ++ni)
	{
		switch (nodeType( *ni))
		{
			case NullNode: break;
			case TermNode:
			{
				const Term& term = m_terms[ nodeIndex( *ni)];
				joinargs.push_back( m_processor->createTermPostingIterator(
							term.type, term.value));
				break;
			}
			case ExpressionNode:
				joinargs.push_back( createExpressionPostingIterator(
							m_expressions[ *ni]));
				break;

		}
	}
	return join->createResultIterator( joinargs, expr.range);
}


PostingIteratorInterface* Query::createNodePostingIterator( const NodeAddress& node)
{
	PostingIteratorInterface* rt = 0;
	switch (nodeType( node))
	{
		case NullNode: break;
		case TermNode:
		{
			const Term& term = m_terms[ nodeIndex( node)];
			rt = m_processor->createTermPostingIterator( term.type, term.value);
			break;
		}
		case ExpressionNode:
			rt = createExpressionPostingIterator( m_expressions[ nodeIndex( node)]);
			break;
	}
	return rt;
}

std::vector<ResultDocument> Query::evaluate()
{
	// [1] Check initial conditions:
	if (m_minRank >= m_maxNofRanks)
	{
		return std::vector<ResultDocument>();
	}
	if (!m_queryEval->weightingFunction().function)
	{
		throw std::runtime_error("cannot evaluate query, no weighting function defined");
	}

	// [2] Define structures needed for query evaluation:
	boost::scoped_ptr<AttributeReaderInterface>
		attributeReader( m_storage->createAttributeReader());

	// [3] Create the posting sets of the query features:
	std::vector<Reference<PostingIteratorInterface> > postings;
	std::map<NodeAddress,std::size_t> featurePostingsMap;

	std::vector<Feature>::const_iterator
		fi = m_features.begin(), fe = m_features.end();
	for (; fi != fe; ++fi)
	{
		Reference<PostingIteratorInterface> postingsElem(
			createNodePostingIterator( fi->node));
		postings.push_back( postingsElem);
		featurePostingsMap[ fi->node] = postings.size()-1;
	}

	// [4] Create the accumulator:
	Accumulator accumulator(
		m_storage,
		m_queryEval->weightingFunction().function, 
		m_queryEval->weightingFunction().parameters,
		m_metaDataReader.get(), m_metaDataRestrictions,
		m_maxNofRanks, m_storage->maxDocumentNumber());

	// [4.1] Add document selection postings:
	std::vector<std::string>::const_iterator
		si = m_queryEval->weightingFunction().selectorSets.begin(),
		se = m_queryEval->weightingFunction().selectorSets.end();
	for (int sidx=0; si != se; ++si,++sidx)
	{
		fi = m_features.begin(), fe = m_features.end();
		for (; fi != fe; ++fi)
		{
			if (*si == fi->set)
			{
				std::size_t pidx = featurePostingsMap.find( fi->node)->second;
				accumulator.addSelector(
					postings[ pidx].get(),
					sidx, nodeType( fi->node) == ExpressionNode);
			}
		}
	}
	// [4.2] Add features for weighting:
	std::vector<std::string>::const_iterator
		wi = m_queryEval->weightingFunction().weightingSets.begin(),
		we = m_queryEval->weightingFunction().weightingSets.end();
	for (; wi != we; ++wi)
	{
		fi = m_features.begin(), fe = m_features.end();
		for (; fi != fe; ++fi)
		{
			if (*wi == fi->set)
			{
				std::size_t pidx = featurePostingsMap.find( fi->node)->second;
				accumulator.addFeature( postings[ pidx].get(), fi->weight);
			}
		}
	}
	// [4.3] Define the user ACL restrictions:
	Reference<InvAclIteratorInterface> invAcl(
		m_storage->createInvAclIterator( m_username));
	if (invAcl.get())
	{
		accumulator.addAclRestriction( invAcl.get());
	}
	// [4.4] Define the feature restrictions:
	std::vector<std::string>::const_iterator
		xi = m_featureRestrictions.begin(), xe = m_featureRestrictions.end();
	for (; xi != xe; ++xi)
	{
		fi = m_features.begin(), fe = m_features.end();
		for (; fi != fe; ++fi)
		{
			if (*xi == fi->set)
			{
				std::size_t pidx = featurePostingsMap.find( fi->node)->second;
				accumulator.addFeatureRestriction(
					postings[ pidx].get(), nodeType( fi->node) == ExpressionNode);
			}
		}
	}

	// [5] Get the summarizers:
	std::vector<Reference<SummarizerClosureInterface> > summarizers;
	std::vector<Reference<PostingIteratorInterface> > allSummarizerPostings;

	std::vector<QueryEval::SummarizerDef>::const_iterator
		zi = m_queryEval->summarizers().begin(),
		ze = m_queryEval->summarizers().end();
	for (; zi != ze; ++zi)
	{
		PostingIteratorInterface* summarizerStructitr = 0;
		std::vector<PostingIteratorInterface*> summarizerPostings;

		// [5.1] Get the summarizer structure feature:
		std::vector<NodeAddress> struct_nodes;
		fi = m_features.begin(), fe = m_features.end();
		for (; fi != fe; ++fi)
		{
			if (zi->structSet == fi->set)
			{
				struct_nodes.push_back( fi->node);
			}
		}
		if (!struct_nodes.empty())
		{
			if (struct_nodes.size() == 1)
			{
				allSummarizerPostings.push_back(
					createNodePostingIterator( struct_nodes[0]));
			}
			else
			{
				Expression expr( Constants::operator_set_union(), struct_nodes);
				allSummarizerPostings.push_back(
					createExpressionPostingIterator( expr));
			}
			summarizerStructitr = allSummarizerPostings.back().get();
		}

		// [5.2] Get the summarizer matching features:
		std::vector<std::string>::const_iterator
			ti = zi->featureSet.begin(), te = zi->featureSet.end();
		for (; ti != te; ++ti)
		{
			fi = m_features.begin(), fe = m_features.end();
			for (; fi != fe; ++fi)
			{
				if (*ti == fi->set)
				{
					allSummarizerPostings.push_back(
						createNodePostingIterator( fi->node));
					summarizerPostings.push_back(
						allSummarizerPostings.back().get());
				}
			}
		}
		summarizers.push_back(
			zi->function->createClosure(
				m_storage, zi->contentType.c_str(), summarizerStructitr,
				summarizerPostings, m_metaDataReader.get(), zi->parameters));
	}

	// [6] Do the Ranking and build the result:
	std::vector<ResultDocument> rt;
	Ranker ranker( m_maxNofRanks);

	Index docno = 0;
	unsigned int state = 0;
	unsigned int prev_state = 0;
	float weight = 0.0;

	while (accumulator.nextRank( docno, state, weight))
	{
		ranker.insert( WeightedDocument( docno, weight));
		if (state > prev_state && ranker.nofRanks() >= m_maxNofRanks)
		{
			break;
		}
		prev_state = state;
	}
	std::vector<WeightedDocument>
		resultlist = ranker.result( m_minRank);
	std::vector<WeightedDocument>::const_iterator
		ri=resultlist.begin(),re=resultlist.end();

	for (; ri != re; ++ri)
	{
		std::vector<ResultDocument::Attribute> attr;
		std::vector<Reference<SummarizerClosureInterface> >::iterator
			si = summarizers.begin(), se = summarizers.end();
		for (std::size_t sidx=0; si != se; ++si,++sidx)
		{
			std::vector<std::string> summary = (*si)->getSummary( ri->docno());
			std::vector<std::string>::const_iterator
				ci = summary.begin(), ce = summary.end();
			for (; ci != ce; ++ci)
			{
				attr.push_back(
					ResultDocument::Attribute(
						m_queryEval->summarizers()[sidx].resultAttribute, *ci));
			}
		}
		rt.push_back( ResultDocument( *ri, attr));
	}
	return rt;
}


