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

namespace strus
{
/// \brief Forward declaration
class QueryInterface;
/// \brief Forward declaration
class StorageInterface;
/// \brief Forward declaration
class SummarizerConfigInterface;
/// \brief Forward declaration
class WeightingConfigInterface;

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
	virtual void defineTerm(
			const std::string& set_,
			const std::string& type_,
			const std::string& value_)=0;

	/// \brief Declare a set of features to be used for selection (declare what documents to weight)
	/// \param[in] set_ name of the set of the selecting feature.
	/// \remark If no selector feature is specified then the weighting features are used for selection
	virtual void defineSelectionFeature( const std::string& set_)=0;

	/// \brief Define a set of features to be used as restriction (exclude documents that do not contain a feature of the set declared)
	/// \param[in] set_ name of the set of the restriction feature
	virtual void defineRestrictionFeature( const std::string& set_)=0;

	/// \brief Declare a set of features to be used for weighting (declare what features to weight)
	/// \param[in] set_ name of the set of the weighting feature.
	/// \remark If no weighhting feature is specified then the query evaluation will allways return an empty ranklist
	virtual void defineWeightingFeature( const std::string& set_)=0;

	/// \brief Create a summarizer to configure for this query evaluation
	/// \param[in] resultAttribute specifies the attribute name this summarization is labeled with in the query evaluation result
	/// \param[in] functionName name of the summarizer function to use. The name references a function defined with QueryProcessor::defineSummarizerFunction(const char*,const SummarizerFunctionInterface*)
	/// \return the summarizer configuration object to be destroyed with 'delete' by the caller
	virtual SummarizerConfigInterface* createSummarizerConfig(
			const std::string& resultAttribute,
			const std::string& functionName)=0;

	/// \brief Create the weighting function to configure for this query evaluation
	/// \param[in] functionName name of the weighting function to use. The name references a function defined with QueryProcessor::defineWeightingFunction(const char*,const WeightingFunctionInterface*)
	/// \return the summarizer configuration object to be destroyed with 'delete' by the caller
	virtual WeightingConfigInterface* createWeightingConfig(
			const std::string& functionName)=0;

	/// \brief Create a new query
	/// \param[in] storage storage to run the query on
	virtual QueryInterface* createQuery( const StorageInterface* storage) const=0;
};

}//namespace
#endif

