/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for defining a query evaluation scheme.
/// \file "queryEvalInterface.hpp"
#ifndef _STRUS_QUERY_EVAL_INTERFACE_HPP_INCLUDED
#define _STRUS_QUERY_EVAL_INTERFACE_HPP_INCLUDED
#include <iostream>
#include <vector>

namespace strus
{
/// \brief Forward declaration
class QueryInterface;
/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class WeightingFunctionInstanceInterface;
/// \brief Forward declaration
class SummarizerFunctionInstanceInterface;
/// \brief Forward declaration
class QueryProcessorInterface;

/// \brief Defines a query evaluation scheme
class QueryEvalInterface
{
public:
	/// \brief Destructor
	virtual ~QueryEvalInterface(){}

	/// \brief Declare a predefined term for all queries of this type. 
	/// \param[in] set_ name of the set of this term. The set is the identifier to address a group of terms. It declares the role of the term in the query
	/// \param[in] type_ name of the type of the term
	/// \param[in] value_ value of the term
	/// \note This function is useful to define structural elements in the document used for query evaluation, that are not part of the query itself. For example sentence or paragraph marker.
	virtual void addTerm(
			const std::string& set_,
			const std::string& type_,
			const std::string& value_)=0;

	/// \brief Declare a set of features to be used for selection (declare what documents to weight)
	/// \param[in] set_ name of the set of the selecting feature.
	/// \remark If no selector feature is specified then the query evaluation fails
	virtual void addSelectionFeature( const std::string& set_)=0;

	/// \brief Define a set of features to be used as restriction (exclude documents that do not contain a feature of the set declared)
	/// \param[in] set_ name of the set of the restriction feature
	virtual void addRestrictionFeature( const std::string& set_)=0;

	/// \brief Define a set of features to be used as exclusion (exclude documents that contain a feature of the set declared)
	/// \param[in] set_ name of the set of the exclusion feature
	virtual void addExclusionFeature( const std::string& set_)=0;

	/// \class FeatureParameter
	/// \brief Structure that describes a feature that is subject of summarization or weighting
	class FeatureParameter
	{
	public:
		FeatureParameter(
				const std::string& parameterName_,
				const std::string& featureSet_)
			:m_parameterName(parameterName_),m_featureSet(featureSet_){}
		FeatureParameter(
				const FeatureParameter& o)
			:m_parameterName(o.m_parameterName),m_featureSet(o.m_featureSet){}

		const std::string& parameterName() const	{return m_parameterName;}
		const std::string& featureSet() const		{return m_featureSet;}

	private:
		std::string m_parameterName;
		std::string m_featureSet;
	};

	/// \brief Declare a summarizer function to use for this query evaluation scheme
	/// \param[in] functionName name of the summarizer function (no meaning, just for inspection and tracing)
	/// \param[in] function parameterized summarizer function to use (ownership passed to this). The function instance can be constructed by getting the function by name from the query processor and parameterizing a created instance of it.
	/// \param[in] featureParameters list of parameters adressing query features that are subject of sumarization
	virtual void addSummarizerFunction(
			const std::string& functionName,
			SummarizerFunctionInstanceInterface* function,
			const std::vector<FeatureParameter>& featureParameters)=0;

	/// \brief Declare a weighting function to use for this query evaluation scheme
	/// \param[in] functionName name of the weighting function (no meaning, just for inspection and tracing)
	/// \param[in] function parameterized weighting function to use (ownership passed to this). The function instance can be constructed by getting the function by name from the query processor and parameterizing a created instance of it.
	/// \param[in] featureParameters list of parameters adressing query features that are subject of weighting
	/// \param[in] weight part of the weight of this function in the total weight sum
	virtual void addWeightingFunction(
			const std::string& functionName,
			WeightingFunctionInstanceInterface* function,
			const std::vector<FeatureParameter>& featureParameters,
			float weight)=0;

	/// \brief Create a new query
	/// \param[in] storage storage to run the query on
	/// \return a query instance for this query evaluation type
	virtual QueryInterface* createQuery(
			const StorageClientInterface* storage) const=0;
};

}//namespace
#endif

