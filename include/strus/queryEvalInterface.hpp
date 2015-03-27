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
class SummarizerConfig;
/// \brief Forward declaration
class WeightingConfig;

/// \brief Defines a program for evaluating a query
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

	/// \brief Declare a summarizer for this query evaluation
	/// \param[in] resultAttribute specifies the attribute name this summarization is labeled with in the query evaluation result
	/// \param[in] functionName name of the summarizer function to use. The name references a function defined with QueryProcessor::defineSummarizerFunction(const char*,const SummarizerFunctionInterface*)
	/// \param[in] config the configuration meaning parametrization of the summarizer declared
	/// \return the summarizer configuration object to be destroyed with 'delete' by the caller
	virtual void addSummarizer(
			const std::string& resultAttribute,
			const std::string& functionName,
			const SummarizerConfig& config)=0;

	/// \brief Declare a weighting function for this query evaluation
	/// \param[in] functionName name of the weighting function to use. The name references a function defined with QueryProcessor::defineWeightingFunction(const char*,const WeightingFunctionInterface*)
	/// \param[in] config the configuration meaning parametrization of the weighting function declared
	/// \param[in] weightedFeatureSets list of feature sets used by this function for weighting (declares what features to weight)
	/// \return the summarizer configuration object to be destroyed with 'delete' by the caller
	virtual void addWeightingFunction(
			const std::string& functionName,
			const WeightingConfig& config,
			const std::vector<std::string>& weightedFeatureSets)=0;

	/// \brief Create a new query
	/// \param[in] storage storage to run the query on
	/// \return a query instance for this query evaluation type
	virtual QueryInterface* createQuery( const StorageClientInterface* storage) const=0;
};

}//namespace
#endif

