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

#undef STRUS_LOWLEVEL_DEBUG

using namespace strus;

///\brief Constructor
Query::Query( const QueryEval* queryEval_, const StorageInterface* storage_, const QueryProcessorInterface* processor_)
	:m_queryEval(queryEval_)
	,m_storage(storage_)
	,m_processor(processor_)
	,m_metaDataReader(storage_->createMetaDataReader())
{
	std::vector<TermConfig>::const_iterator
		ti = m_queryEval->terms().begin(),
		te = m_queryEval->terms().end();
	for (; ti != te; ++ti)
	{
		pushTerm( ti->type, ti->value);
		defineFeature( ti->set, 1.0);
	}
}

Query::Query( const Query& o)
	:m_queryEval(o.m_queryEval)
	,m_storage(o.m_storage)
	,m_processor(o.m_processor)
	,m_metaDataReader(o.m_metaDataReader)
	,m_terms(o.m_terms)
	,m_expressions(o.m_expressions)
	,m_features(o.m_features)
	,m_stack(o.m_stack)
	,m_metaDataRestrictions(o.m_metaDataRestrictions)
	,m_variableAssignments(o.m_variableAssignments)
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

void Query::attachVariable( const std::string& name_)
{
	if (m_stack.empty())
	{
		throw std::runtime_error( "cannot attach variable (query stack empty)");
	}
	m_variableAssignments.insert( std::pair<NodeAddress,std::string>( m_stack.back(), name_));
}

void Query::defineFeature( const std::string& set_, float weight_)
{
	m_features.push_back( Feature( boost::algorithm::to_lower_copy(set_), m_stack.back(), weight_));
	m_stack.pop_back();
}

void Query::defineMetaDataRestriction(
		CompareOperator opr, const std::string&  name,
		const ArithmeticVariant& operand, bool newGroup)
{
	Index hnd = m_metaDataReader->elementHandle( name);
	const char* typeName = m_metaDataReader->getType( hnd);
	m_metaDataRestrictions.push_back( MetaDataRestriction( typeName, opr, hnd, operand, newGroup));
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

void Query::printVariables( std::ostream& out, NodeAddress adr)
{
	typedef std::multimap<NodeAddress,std::string>::const_iterator Itr;
	std::pair<Itr,Itr> variables = m_variableAssignments.equal_range( adr);

	if (variables.first != variables.second)
	{
		out << " [";
		Itr vi = variables.first, ve = variables.second;
		for (std::size_t vidx=0; vi != ve; ++vi,++vidx)
		{
			if (vidx) out << ",";
			out << vi->second;
		}
		out << "]";
	}
}

void Query::printNode( std::ostream& out, NodeAddress adr, std::size_t indent)
{
	std::string indentstr( indent*2, ' ');
	switch (nodeType( adr))
	{
		case NullNode:
			out << indentstr << "NULL";
			printVariables( out, adr);
			out << std::endl;
			break;
		case TermNode:
		{
			const Term& term = m_terms[ nodeIndex( adr)];
			out << indentstr << "term " << term.type << " '" << term.value << "'";
			printVariables( out, adr);
			out << std::endl;
			break;
		}
		case ExpressionNode:
		{
			const Expression& expr = m_expressions[ nodeIndex( adr)];
			out << indentstr << expr.opname << " " << expr.range << ":";
			printVariables( out, adr);
			out << std::endl;
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
			case NullNode:
				break;
			case TermNode:
			{
				const Term& term = m_terms[ nodeIndex( *ni)];
				joinargs.push_back( m_processor->createTermPostingIterator(
							term.type, term.value));
				m_nodePostingsMap[ *ni] = joinargs.back().get();
				break;
			}
			case ExpressionNode:
				joinargs.push_back( createExpressionPostingIterator(
							m_expressions[ *ni]));
				m_nodePostingsMap[ *ni] = joinargs.back().get();
				break;
		}
	}
	return join->createResultIterator( joinargs, expr.range);
}


PostingIteratorInterface* Query::createNodePostingIterator( const NodeAddress& nodeadr)
{
	PostingIteratorInterface* rt = 0;
	switch (nodeType( nodeadr))
	{
		case NullNode: break;
		case TermNode:
		{
			std::size_t nidx = nodeIndex( nodeadr);
			const Term& term = m_terms[ nidx];
			rt = m_processor->createTermPostingIterator( term.type, term.value);
			m_nodePostingsMap[ nodeadr] = rt;
			break;
		}
		case ExpressionNode:
			std::size_t nidx = nodeIndex( nodeadr);
			rt = createExpressionPostingIterator( m_expressions[ nidx]);
			m_nodePostingsMap[ nodeadr] = rt;
			break;
	}
	return rt;
}

PostingIteratorInterface* Query::nodePostings( const NodeAddress& nodeadr) const
{
	std::map<NodeAddress,PostingIteratorInterface*>::const_iterator
		pi = m_nodePostingsMap.find( nodeadr);
	if (pi == m_nodePostingsMap.end())
	{
		throw std::runtime_error("internal: expression node postings not found");
	}
	return pi->second;
}

void Query::collectSummarizationVariables(
			std::vector<SummarizationVariable>& variables,
			const NodeAddress& nodeadr)
{
	typedef std::multimap<NodeAddress,std::string>::const_iterator Itr;
	std::pair<Itr,Itr> vrange = m_variableAssignments.equal_range( nodeadr);

	Itr vi = vrange.first, ve = vrange.second;
	for (; vi != ve; ++vi)
	{
		variables.push_back( SummarizationVariable( vi->second, nodePostings( nodeadr)));
	}

	switch (nodeType( nodeadr))
	{
		case NullNode: break;
		case TermNode: break;
		case ExpressionNode:
		{
			std::size_t nidx = nodeIndex( nodeadr);
			const Expression& expr = m_expressions[ nidx];
			std::vector<NodeAddress>::const_iterator
				ni = expr.subnodes.begin(), 
				ne = expr.subnodes.end();
			collectSummarizationVariables( variables, *ni);
			break;
		}
	}
}

SummarizationFeature Query::createSummarizationFeature(
			const NodeAddress& nodeadr)
{
	std::vector<SummarizationVariable> variables;
	collectSummarizationVariables( variables, nodeadr);
	return SummarizationFeature( nodePostings(nodeadr), variables);
}


std::vector<ResultDocument> Query::evaluate()
{
#ifdef STRUS_LOWLEVEL_DEBUG
	std::cout << "evaluate query:" << std::endl;
	print( std::cout);
#endif

	// [1] Check initial conditions:
	if (m_minRank >= m_maxNofRanks)
	{
		return std::vector<ResultDocument>();
	}
	if (!m_queryEval->weighting().function())
	{
		throw std::runtime_error("cannot evaluate query, no weighting function defined");
	}

	// [3] Create the posting sets of the query features:
	std::vector<Reference<PostingIteratorInterface> > postings;

	std::vector<Feature>::const_iterator
		fi = m_features.begin(), fe = m_features.end();
	for (; fi != fe; ++fi)
	{
		Reference<PostingIteratorInterface> postingsElem(
			createNodePostingIterator( fi->node));
		postings.push_back( postingsElem);
	}

	// [4] Create the accumulator:
	Accumulator accumulator(
		m_storage,
		m_queryEval->weighting().function(), 
		m_queryEval->weighting().parameters(),
		m_metaDataReader.get(), m_metaDataRestrictions,
		m_maxNofRanks, m_storage->maxDocumentNumber());

	// [4.1] Add document selection postings:
	std::vector<std::string>::const_iterator
		si = m_queryEval->selectionSets().begin(),
		se = m_queryEval->selectionSets().end();
	if (si == se)
	{
		si = m_queryEval->weightingSets().begin(),
		se = m_queryEval->weightingSets().end();
	}
	for (int sidx=0; si != se; ++si,++sidx)
	{
		fi = m_features.begin(), fe = m_features.end();
		for (; fi != fe; ++fi)
		{
			if (*si == fi->set)
			{
				accumulator.addSelector(
					nodePostings( fi->node), sidx, 
					nodeType( fi->node) == ExpressionNode);
			}
		}
	}

	// [4.2] Add features for weighting:
	std::vector<std::string>::const_iterator
		wi = m_queryEval->weightingSets().begin(),
		we = m_queryEval->weightingSets().end();
	for (; wi != we; ++wi)
	{
		fi = m_features.begin(), fe = m_features.end();
		for (; fi != fe; ++fi)
		{
			if (*wi == fi->set)
			{
				accumulator.addFeature( nodePostings( fi->node), fi->weight);
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
		xi = m_queryEval->restrictionSets().begin(),
		xe = m_queryEval->restrictionSets().end();
	for (; xi != xe; ++xi)
	{
		fi = m_features.begin(), fe = m_features.end();
		for (; fi != fe; ++fi)
		{
			if (*xi == fi->set)
			{
				accumulator.addFeatureRestriction(
					nodePostings( fi->node),
					nodeType( fi->node) == ExpressionNode);
			}
		}
	}

	// [5] Create the summarizers:
	std::vector<Reference<SummarizerClosureInterface> > summarizers;
	std::vector<SummarizerDef>::const_iterator
		zi = m_queryEval->summarizers().begin(),
		ze = m_queryEval->summarizers().end();
	for (; zi != ze; ++zi)
	{
		// [5.1] Collect the summarization features:
		std::vector<SummarizerFunctionInterface::FeatureParameter> summarizeFeatures;
		std::vector<SummarizerDef::Feature>::const_iterator
			si = zi->featureParameters().begin(),
			se = zi->featureParameters().end();
		for (; si != se; ++si)
		{
			fi = m_features.begin(), fe = m_features.end();
			for (; fi != fe; ++fi)
			{
				if (fi->set == si->set)
				{
					SummarizationFeature
						sumfeat = createSummarizationFeature( fi->node);

					summarizeFeatures.push_back(
						SummarizerFunctionInterface::FeatureParameter(
							si->classidx, sumfeat));
				}
			}
		}
		// [5.2] Create the summarizer:
		summarizers.push_back(
			zi->function()->createClosure(
				m_storage, m_processor, m_metaDataReader.get(),
				summarizeFeatures, zi->textualParameters(), zi->numericParameters()));
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
			std::vector<SummarizerClosureInterface::SummaryElement>
				summary = (*si)->getSummary( ri->docno());
			std::vector<SummarizerClosureInterface::SummaryElement>::const_iterator
				ci = summary.begin(), ce = summary.end();
			for (; ci != ce; ++ci)
			{
				attr.push_back(
					ResultDocument::Attribute(
						m_queryEval->summarizers()[sidx].resultAttribute(),
						ci->text(), ci->weight()));
			}
		}
		rt.push_back( ResultDocument( *ri, attr));
	}
	return rt;
}


