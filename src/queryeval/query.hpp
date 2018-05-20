/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_QUERY_HPP_INCLUDED
#define _STRUS_QUERY_HPP_INCLUDED
#include "strus/queryInterface.hpp"
#include "strus/summarizationVariable.hpp"
#include "strus/reference.hpp"
#include "private/internationalization.hpp"
#include "strus/metaDataRestrictionInterface.hpp"
#include "strus/scalarFunctionInstanceInterface.hpp"
#include <vector>
#include <string>
#include <map>
#include <utility>
#include <iostream>
#include <stdexcept>

namespace strus {

/// \brief Forward declaration
class QueryProcessorInterface;
/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class QueryEval;
/// \brief Forward declaration
class PostingIteratorInterface;
/// \brief Forward declaration
class ErrorBufferInterface;
/// \brief Forward declaration
class DebugTraceContextInterface;

/// \brief Implementation of the query interface
class Query
	:public QueryInterface
{
public:
	///\brief Constructor
	Query(
			const QueryEval* queryEval_,
			const StorageClientInterface* storage_,
			ErrorBufferInterface* errorhnd_);

	virtual ~Query();

	virtual void pushTerm(
			const std::string& type_,
			const std::string& value_,
			const Index& length_);
	virtual void pushDocField(
			const std::string& metadataRangeStart,
			const std::string& metadataRangeEnd);
	virtual void pushExpression(
			const PostingJoinOperatorInterface* operation,
			unsigned int argc, int range_, unsigned int cardinality_);

	virtual void attachVariable(
			const std::string& name_);
	virtual void defineFeature(
			const std::string& set_,
			double weight_=1.0);

	virtual void addMetaDataRestrictionCondition(
			const MetaDataRestrictionInterface::CompareOperator& opr,
			const std::string& name,
			const NumericVariant& operand,
			bool newGroup);

	virtual void addDocumentEvaluationSet(
			const std::vector<Index>& docnolist_);

	virtual void addAccess( const std::string& username_);

	virtual void setMaxNofRanks( std::size_t nofRanks_);
	virtual void setMinRank( std::size_t minRank_);

	virtual void defineTermStatistics(
			const std::string& type_,
			const std::string& value_,
			const TermStatistics& stats_);
	virtual void defineGlobalStatistics(
			const GlobalStatistics& stats_);

	virtual void setWeightingVariableValue(
			const std::string& name, double value);

	virtual void setDebugMode( bool debug);

	virtual QueryResult evaluate() const;
	virtual std::string tostring() const;

public:
	typedef unsigned int NodeAddress;

	enum NodeType
	{
		NullNode,
		TermNode,
		DocFieldNode,
		ExpressionNode
	};
	enum {NodeTypeShift=28, NodeTypeMask=0x7, NodeIndexMask=(1<<NodeTypeShift)-1};

	static const char* nodeTypeName( NodeType i)
	{
		static const char* ar[] = {"NullNode","TermNode","DocFieldNode","ExpressionNode"};
		return ar[i];
	}

	static NodeType nodeType( NodeAddress nd)
	{
		return (NodeType)((nd >> NodeTypeShift) & NodeTypeMask);
	}
	static std::size_t nodeIndex( NodeAddress nd)
	{
		return (std::size_t)(nd & NodeIndexMask);
	}
	NodeAddress nodeAddress( NodeType type, std::size_t idx)
	{
		return (NodeAddress)(((unsigned int)type << NodeTypeShift) | (unsigned int)idx);
	}

	struct TermKey
	{
		TermKey( const TermKey& o)
			:type(o.type),value(o.value){}
		TermKey( const std::string& t, const std::string& v)
			:type(t),value(v){}

		bool operator<( const TermKey& o) const;

		std::string type;	///< term type name
		std::string value;	///< term value
	};

	struct Term
	{
		Term( const Term& o)
			:type(o.type),value(o.value),length(o.length){}
		Term( const std::string& t, const std::string& v, const Index& l)
			:type(t),value(v),length(l){}

		std::string type;	///< term type name
		std::string value;	///< term value
		Index length;		///< term length (ordinal position count)
	};

	/// \brief Metadata defined postings specifying a single area in the document 
	struct DocField
	{
		DocField( const DocField& o)
			:metadataRangeStart(o.metadataRangeStart),metadataRangeEnd(o.metadataRangeEnd){}
		DocField( const std::string& start, const std::string& end)
			:metadataRangeStart(start),metadataRangeEnd(end){}

		std::string metadataRangeStart;		///< metadata element defining the start of the field
		std::string metadataRangeEnd;		///< metadata element defining the end of the field
	};

	struct Expression
	{
		Expression(
				const PostingJoinOperatorInterface* operation_,
				const std::vector<NodeAddress>& subnodes_,
				int range_,
				unsigned int cardinality_)
			:operation(operation_),subnodes(subnodes_),range(range_),cardinality(cardinality_){}
		Expression(
				const Expression& o)
			:operation(o.operation),subnodes(o.subnodes),range(o.range),cardinality(o.cardinality){}

		const PostingJoinOperatorInterface* operation;
		std::vector<NodeAddress> subnodes;
		int range;
		unsigned int cardinality;
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

	void print( std::ostream& out) const;

private:
	const TermStatistics& getTermStatistics( const std::string& type_, const std::string& value_) const;

	struct NodeStorageData
	{
		explicit NodeStorageData( PostingIteratorInterface* itr_=0)
			:itr(itr_),stats(){}
		NodeStorageData( PostingIteratorInterface* itr_, TermStatistics stats_)
			:itr(itr_),stats(stats_){}
		NodeStorageData( const NodeStorageData& o)
			:itr(o.itr),stats(o.stats){}

		PostingIteratorInterface* itr;
		TermStatistics stats;
	};

	typedef std::map<NodeAddress,NodeStorageData> NodeStorageDataMap;

	struct WeightingVariableValueAssignment
	{
		std::string varname;
		std::size_t index;
		double value;

		WeightingVariableValueAssignment( const std::string& varname_, std::size_t index_, double value_)
			:varname(varname_),index(index_),value(value_){}
		WeightingVariableValueAssignment( const WeightingVariableValueAssignment& o)
			:varname(o.varname),index(o.index),value(o.value){}
	};

	PostingIteratorInterface* createExpressionPostingIterator( const Expression& expr, NodeStorageDataMap& nodeStorageDataMap) const;
	PostingIteratorInterface* createNodePostingIterator( const NodeAddress& nodeadr, NodeStorageDataMap& nodeStorageDataMap) const;
	void collectSummarizationVariables(
				std::vector<SummarizationVariable>& variables,
				const NodeAddress& nodeadr,
				const NodeStorageDataMap& nodeStorageDataMap) const;
	const NodeStorageData& nodeStorageData( const NodeAddress& nodeadr, const NodeStorageDataMap& nodeStorageDataMap) const;

	void printNode( std::ostream& out, NodeAddress adr, std::size_t indent) const;
	void printVariables( std::ostream& out, NodeAddress adr) const;
	NodeAddress duplicateNode( NodeAddress adr);
	std::string printStack() const;

private:
	const QueryEval* m_queryEval;
	const StorageClientInterface* m_storage;
	mutable Reference<MetaDataReaderInterface> m_metaDataReader;
	Reference<MetaDataRestrictionInterface> m_metaDataRestriction;	///< restriction function on metadata
	Reference<ScalarFunctionInstanceInterface> m_weightingFormula;	///< instance of the scalar function to calculate the weight of a document from the weighting functions defined as parameter
	std::vector<Term> m_terms;					///< query terms
	std::vector<DocField> m_docfields;				///< fields in document defined by meta data
	std::vector<Expression> m_expressions;				///< query expressions
	std::vector<Feature> m_features;				///< query features
	std::vector<NodeAddress> m_stack;				///< expression build stack
	std::multimap<NodeAddress,std::string> m_variableAssignments;	///< maps node addresses to names of variables attached to
	std::size_t m_nofRanks;						///< number of ranks to evaluate
	std::size_t m_minRank;						///< smallest rank to return (start of result ranklist -- browsing)
	std::vector<std::string> m_usernames;				///< users allowed to see the query result
	std::vector<Index> m_evalset_docnolist;				///< set of document numbers to restrict the query to
	bool m_evalset_defined;						///< true, if the set of document numbers to restrict the query to is defined
	typedef std::map<TermKey,TermStatistics> TermStatisticsMap;
	TermStatisticsMap m_termstatsmap;				///< term statistics (evaluation in case of a distributed index)
	GlobalStatistics m_globstats;					///< global statistics (evaluation in case of a distributed index)
	std::vector<WeightingVariableValueAssignment> m_weightingvars;	///< non constant weight variables (defined by query and not the query eval)
	std::vector<WeightingVariableValueAssignment> m_summaryweightvars; ///< non constant summarization weight variables (defined by query and not the query eval)
	bool m_debugMode;						///< true if debug mode is enabled
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
	DebugTraceContextInterface* m_debugtrace;			///< debug trace interface
};

}//namespace
#endif

