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
#include <map>

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
		:m_weightingSets(),m_selectionSets(),m_restrictionSets()
		,m_exclusionSets(),m_weightingFunctions(),m_summarizers()
		,m_weightingFormula(),m_terms(),m_varassignmap()
		,m_usePosinfo(true),m_errorhnd(errorhnd_){}

	QueryEval( const QueryEval& o)
		:m_weightingSets(o.m_weightingSets)
		,m_selectionSets(o.m_selectionSets)
		,m_restrictionSets(o.m_restrictionSets)
		,m_exclusionSets(o.m_exclusionSets)
		,m_weightingFunctions(o.m_weightingFunctions)
		,m_summarizers(o.m_summarizers)
		,m_weightingFormula(o.m_weightingFormula)
		,m_terms(o.m_terms)
		,m_varassignmap(o.m_varassignmap)
		,m_usePosinfo(o.m_usePosinfo)
		,m_errorhnd(o.m_errorhnd)
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

	virtual std::vector<std::string> getWeightingFeatureSets() const;
	virtual std::vector<std::string> getSelectionFeatureSets() const;
	virtual std::vector<std::string> getRestrictionFeatureSets() const;
	virtual std::vector<std::string> getExclusionFeatureSets() const;
	
	virtual void addSummarizerFunction(
			const std::string& summaryId,
			SummarizerFunctionInstanceInterface* function,
			const std::vector<FeatureParameter>& featureParameters,
			const std::string& debugAttributeName);

	virtual void addWeightingFunction(
			WeightingFunctionInstanceInterface* function,
			const std::vector<FeatureParameter>& featureParameters,
			const std::string& debugAttributeName);

	virtual void defineWeightingFormula(
			ScalarFunctionInterface* combinefunc);

	virtual void usePositionInformation( bool yes)
		{m_usePosinfo = yes;}

	virtual StructView view() const;

public:/*Query*/
	const std::vector<TermConfig>& terms() const			{return m_terms;}
	const std::vector<SummarizerDef>& summarizers() const		{return m_summarizers;}
	const std::vector<std::string>& selectionSets() const		{return m_selectionSets;}
	const std::vector<std::string>& restrictionSets() const		{return m_restrictionSets;}
	const std::vector<std::string>& exclusionSets() const		{return m_exclusionSets;}
	const std::vector<WeightingDef>& weightingFunctions() const	{return m_weightingFunctions;}
	const ScalarFunctionInterface* weightingFormula() const		{return m_weightingFormula.get();}

public:/*Query*/
	struct VariableAssignment
	{
		enum Target {WeightingFunction, SummarizerFunction, FormulaFunction};
		Target target;
		std::size_t index;

		VariableAssignment( Target target_, std::size_t index_)
			:target(target_),index(index_){}
		VariableAssignment( const VariableAssignment& o)
			:target(o.target),index(o.index){}
	};
	std::vector<VariableAssignment>
		weightingVariableAssignmentList(
			const std::string& varname) const;

private:
	void defineVariableAssignments( const std::vector<std::string>& variables, VariableAssignment::Target target, std::size_t index);

private:
	std::vector<std::string> m_weightingSets;			///< posting sets used for weighting the documents matched
	std::vector<std::string> m_selectionSets;			///< posting sets selecting the documents to match
	std::vector<std::string> m_restrictionSets;			///< posting sets restricting the documents to match
	std::vector<std::string> m_exclusionSets;			///< posting sets excluding the documents to match
	std::vector<WeightingDef> m_weightingFunctions;			///< weighting function configuration
	std::vector<SummarizerDef> m_summarizers;			///< list of summarizer configurations
	Reference<ScalarFunctionInterface> m_weightingFormula;		///< scalar function to calculate the weight of a document from the weighting functions defined as parameter

	std::vector<TermConfig> m_terms;				///< list of predefined terms used in query evaluation but not part of the query (e.g. punctuation)
	std::multimap<std::string,VariableAssignment> m_varassignmap;	///< map of weight variable assignments
	bool m_usePosinfo;						///< true (default) if position info is used for evaluation
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif

