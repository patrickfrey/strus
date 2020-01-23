/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for the context data of a processed weighting function
/// \file weightingFunctionContextInterface.hpp
#ifndef _STRUS_WEIGHTING_EXECUTION_CONTEXT_INTERFACE_HPP_INCLUDED
#define _STRUS_WEIGHTING_EXECUTION_CONTEXT_INTERFACE_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/termStatistics.hpp"
#include "strus/weightedField.hpp"
#include <string>

namespace strus
{
/// \brief Forward declaration
class MetaDataReaderInterface;
/// \brief Forward declaration
class PostingIteratorInterface;

/// \brief Interface for a weighting function with its state and context used during calculation
class WeightingFunctionContextInterface
{
public:
	/// \brief Destructor
	virtual ~WeightingFunctionContextInterface(){}

	/// \brief Add a feature that is subject of weighting to the execution context
	/// \param[in] name_ name of the weighting feature
	/// \param[in] postingIterator_ iterator on the matches of the weighting feature (ownership retained by the caller)
	/// \param[in] weight_ weight of this feature
	/// \param[in] stats_ global term statistics passed down with the query. If undefined, they can be defined by or estimated from the posting iterator data. 
	/// \remark Do call this method before calling call the first time for not having incomplete results
	virtual void addWeightingFeature(
			const std::string& name_,
			PostingIteratorInterface* postingIterator_,
			double weight_,
			const TermStatistics& stats_)=0;

	/// \brief Set the value of a query variable
	/// \param[in] name name of the variable
	/// \param[in] value value of the variable
	virtual void setVariableValue( const std::string& name, double value)=0;

	/// \brief Call the weighting function for a document
	/// \param[in] docno document number
	/// \return the calculated best N weighted fields of the document as reference
	virtual const std::vector<WeightedField>& call( const Index& docno)=0;

	/// \brief Get debug info dumped as string of the weighting call for one document
	/// \param[in] docno document to get the debug info from
	/// \return the debug info as string
	virtual std::string debugCall( const Index& docno)=0;
};

}//namespace
#endif

