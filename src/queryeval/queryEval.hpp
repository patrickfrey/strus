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
#ifndef _STRUS_QUERY_PROGRAM_HPP_INCLUDED
#define _STRUS_QUERY_PROGRAM_HPP_INCLUDED
#include "strus/queryEvalInterface.hpp"
#include "strus/weightedDocument.hpp"
#include "strus/accumulatorInterface.hpp"
#include "parser/joinFunction.hpp"
#include "parser/joinOperation.hpp"
#include "parser/selectorExpression.hpp"
#include "parser/accumulateOperation.hpp"
#include <string>
#include <vector>

namespace strus {

/// \brief Query evaluation program representation
class QueryEval
	:public QueryEvalInterface
{
public:
	enum {MaxSizeFeatureSet=100};

	QueryEval(){}
	QueryEval( const QueryEval& o)
		:m_predefinedTerms(o.m_predefinedTerms)
		,m_selectors(o.m_selectors)
		,m_functions(o.m_functions)
		,m_setnamemap(o.m_setnamemap)
		,m_operations(o.m_operations)
		,m_accumulateOperation(o.m_accumulateOperation)
	{}
	QueryEval( const std::string& source);

	virtual std::vector<WeightedDocument>
		getRankedDocumentList(
			const QueryProcessorInterface& processor,
			const Query& query,
			std::size_t maxNofRanks) const;

	const std::vector<Query::Term>& predefinedTerms() const			{return m_predefinedTerms;}
	const std::vector<parser::SelectorExpression>& selectors() const	{return m_selectors;}
	const std::vector<parser::JoinFunction>& functions() const		{return m_functions;}
	const std::vector<parser::JoinOperation>& operations() const		{return m_operations;}
	const parser::AccumulateOperation& accumulateOperation() const		{return m_accumulateOperation;}

	virtual void print( std::ostream& out) const;

private:
	std::vector<WeightedDocument>
		getRankedDocumentList(
			AccumulatorInterface& accu,
			std::size_t maxNofRanks) const;

	void parseJoinOperationDef( char const* src);
	void parseAccumulatorDef( char const* src);
	void parseTermDef( char const* src);

private:
	std::vector<Query::Term> m_predefinedTerms;
	std::vector<parser::SelectorExpression> m_selectors;
	std::vector<parser::JoinFunction> m_functions;
	StringIndexMap m_setnamemap;
	std::vector<parser::JoinOperation> m_operations;
	parser::AccumulateOperation m_accumulateOperation;
};

}//namespace
#endif

