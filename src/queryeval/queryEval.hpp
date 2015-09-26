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
#include "strus/resultDocument.hpp"
#include "strus/arithmeticVariant.hpp"
#include "termConfig.hpp"
#include "summarizerDef.hpp"
#include "weightingDef.hpp"
#include <string>
#include <vector>

namespace strus {

/// \brief Forward declaration
class SummarizerFunctionInterface;
/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class QueryProcessorInterface;
/// \brief Forward declaration
class WeightingFunctionInterface;
/// \brief Forward declaration
class SummarizerFunctionInterface;
/// \brief Forward declaration
class SummarizerConfig;
/// \brief Forward declaration
class WeightingConfig;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Query evaluation program representation
class QueryEval
	:public QueryEvalInterface
{
public:
	explicit QueryEval( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}

	QueryEval( const QueryEval& o)
		:m_selectionSets(o.m_selectionSets)
		,m_restrictionSets(o.m_restrictionSets)
		,m_exclusionSets(o.m_exclusionSets)
		,m_weightingFunctions(o.m_weightingFunctions)
		,m_summarizers(o.m_summarizers)
		,m_terms(o.m_terms)
	{}

	virtual QueryInterface* createQuery(
			const StorageClientInterface* storage) const;

	virtual void addTerm(
			const std::string& set_,
			const std::string& type_,
			const std::string& value_);

	virtual void addSelectionFeature( const std::string& set_);

	virtual void addRestrictionFeature( const std::string& set_);
	virtual void addExclusionFeature( const std::string& set_);

	virtual void addSummarizerFunction(
			const std::string& functionName,
			SummarizerFunctionInstanceInterface* function,
			const std::vector<FeatureParameter>& featureParameters,
			const std::string& resultAttribute);

	virtual void addWeightingFunction(
			const std::string& functionName,
			WeightingFunctionInstanceInterface* function,
			const std::vector<FeatureParameter>& featureParameters,
			float weight);

	void print( std::ostream& out) const;


public:
	const std::vector<TermConfig>& terms() const			{return m_terms;}
	const std::vector<SummarizerDef>& summarizers() const		{return m_summarizers;}
	const std::vector<std::string>& selectionSets() const		{return m_selectionSets;}
	const std::vector<std::string>& restrictionSets() const		{return m_restrictionSets;}
	const std::vector<std::string>& exclusionSets() const		{return m_exclusionSets;}
	const std::vector<WeightingDef>& weightingFunctions() const	{return m_weightingFunctions;}

private:
	std::vector<std::string> m_selectionSets;	///< posting sets selecting the documents to match
	std::vector<std::string> m_restrictionSets;	///< posting sets restricting the documents to match
	std::vector<std::string> m_exclusionSets;	///< posting sets excluding the documents to match
	std::vector<WeightingDef> m_weightingFunctions;	///< weighting function configuration
	std::vector<SummarizerDef> m_summarizers;	///< list of summarizer configurations
	std::vector<TermConfig> m_terms;		///< list of predefined terms used in query evaluation but not part of the query (e.g. punctuation)
	ErrorBufferInterface* m_errorhnd;		///< buffer for error messages
};

}//namespace
#endif

