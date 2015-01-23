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
#ifndef _STRUS_QUERY_HPP_INCLUDED
#define _STRUS_QUERY_HPP_INCLUDED
#include "strus/queryInterface.hpp"
#include "strus/reference.hpp"
#include "metaDataRestriction.hpp"
#include <vector>
#include <string>
#include <utility>
#include <iostream>
#include <stdexcept>

namespace strus {

/// \brief Forward declaration
class QueryProcessorInterface;
/// \brief Forward declaration
class StorageInterface;
/// \brief Forward declaration
class QueryEval;
/// \brief Forward declaration
class PostingIteratorInterface;

/// \brief Implementation of the query interface
class Query
	:public QueryInterface
{
public:
	///\brief Constructor
	Query( const QueryEval* queryEval_, const StorageInterface* storage_, const QueryProcessorInterface* processor_);

	///\brief Copy constructor
	Query( const Query& o);

	virtual ~Query(){}

	virtual void print( std::ostream& out);

	virtual void pushTerm( const std::string& type_, const std::string& value_);
	virtual void pushExpression( const std::string& opname_, std::size_t argc, int range_);
	virtual void defineFeature( const std::string& set_, float weight_=1.0);

	virtual void defineMetaDataRestriction(
			CompareOperator opr, const char* name,
			const ArithmeticVariant& operand, bool newGroup=true);

	virtual void setMaxNofRanks( std::size_t maxNofRanks_);
	virtual void setMinRank( std::size_t maxNofRanks_);
	virtual void setUserName( const std::string& username_);

	virtual std::vector<ResultDocument> evaluate();

public:
	typedef int NodeAddress;

	enum NodeType
	{
		NullNode,
		TermNode,
		ExpressionNode
	};

	static NodeType nodeType( NodeAddress nd)
	{
		return (nd < 0)?ExpressionNode:((nd == 0)?NullNode:TermNode);
	}
	static std::size_t nodeIndex( NodeAddress nd)
	{
		return (std::size_t)((nd > 0)?(nd-1):(-nd+1));
	}
	NodeAddress nodeAddress( NodeType type, std::size_t idx)
	{
		switch (type)
		{
			case NullNode: return 0;
			case TermNode: return +(int)(idx+1);
			case ExpressionNode: return -(int)(idx+1);
		}
		throw std::logic_error("illegal node address type (query)");
	}

	struct Term
	{
		Term( const Term& o)
			:type(o.type),value(o.value){}
		Term( const std::string& t, const std::string& v)
			:type(t),value(v){}

		std::string type;	///< term type name
		std::string value;	///< term value
	};

	struct Expression
	{
		Expression(
				const std::string& opname_,
				const std::vector<NodeAddress>& subnodes_,
				int range_=0)
			:opname(opname_),subnodes(subnodes_),range(range_){}
		Expression(
				const Expression& o)
			:opname(o.opname),subnodes(o.subnodes),range(o.range){}

		std::string opname;
		std::vector<NodeAddress> subnodes;
		int range;
	};

	struct Feature
	{
		Feature( const std::string& set_, NodeAddress node_, float weight_)
			:set(set_),node(node_),weight(weight_){}
		Feature( const Feature& o)
			:set(o.set),node(o.node),weight(o.weight){}

		std::string set;	///< name of the set the feature belongs to
		NodeAddress node;	///< feature expression tree or single node reference
		float weight;		///< feature weight in ranking
	};

	///\brief Get the joined features defined in the query
	const std::vector<Feature>& features() const	{return m_features;}

private:
	PostingIteratorInterface* createExpressionPostingIterator( const Expression& expr);
	PostingIteratorInterface* createNodePostingIterator( const NodeAddress& node, float weight=0.0);
	void printNode( std::ostream& out, NodeAddress adr, std::size_t indent);

private:
	const QueryEval* m_queryEval;
	const StorageInterface* m_storage;
	const QueryProcessorInterface* m_processor;
	Reference<MetaDataReaderInterface> m_metaDataReader;
	std::vector<Term> m_terms;
	std::vector<Expression> m_expressions;
	std::vector<Feature> m_features;
	std::vector<NodeAddress> m_stack;
	std::vector<MetaDataRestriction> m_restrictions;
	std::size_t m_maxNofRanks;
	std::size_t m_minRank;
	std::string m_username;
};

}//namespace
#endif

