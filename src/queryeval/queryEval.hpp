/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_QUERY_PROGRAM_HPP_INCLUDED
#define _STRUS_QUERY_PROGRAM_HPP_INCLUDED
#include "strus/queryEvalInterface.hpp"
#include "strus/resultDocument.hpp"
#include "strus/scalarFunctionInterface.hpp"
#include "termConfig.hpp"
#include "summarizerDef.hpp"
#include "weightingDef.hpp"
#include <string>
#include <vector>

namespace strus {

/// \brief Forward declaration
class StorageClientInterface;
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
			const std::vector<FeatureParameter>& featureParameters);

	virtual void addWeightingFunction(
			const std::string& functionName,
			WeightingFunctionInstanceInterface* function,
			const std::vector<FeatureParameter>& featureParameters);

	virtual void defineWeightingFormula(
			ScalarFunctionInterface* combinefunc);

	void print( std::ostream& out) const;


public:
	const std::vector<TermConfig>& terms() const			{return m_terms;}
	const std::vector<SummarizerDef>& summarizers() const		{return m_summarizers;}
	const std::vector<std::string>& selectionSets() const		{return m_selectionSets;}
	const std::vector<std::string>& restrictionSets() const		{return m_restrictionSets;}
	const std::vector<std::string>& exclusionSets() const		{return m_exclusionSets;}
	const std::vector<WeightingDef>& weightingFunctions() const	{return m_weightingFunctions;}
	const ScalarFunctionInterface* weightingFormula() const		{return m_weightingFormula.get();}

private:
	std::vector<std::string> m_selectionSets;	///< posting sets selecting the documents to match
	std::vector<std::string> m_restrictionSets;	///< posting sets restricting the documents to match
	std::vector<std::string> m_exclusionSets;	///< posting sets excluding the documents to match
	std::vector<WeightingDef> m_weightingFunctions;	///< weighting function configuration
	std::vector<SummarizerDef> m_summarizers;	///< list of summarizer configurations
	Reference<ScalarFunctionInterface> m_weightingFormula;	///< scalar function to calculate the weight of a document from the weighting functions defined as parameter

	std::vector<TermConfig> m_terms;		///< list of predefined terms used in query evaluation but not part of the query (e.g. punctuation)
	ErrorBufferInterface* m_errorhnd;		///< buffer for error messages
};

}//namespace
#endif

