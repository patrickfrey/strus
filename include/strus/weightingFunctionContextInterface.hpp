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
/// \brief Interface for the context data of a processed weighting function
/// \file weightingFunctionContextInterface.hpp
#ifndef _STRUS_WEIGHTING_EXECUTION_CONTEXT_INTERFACE_HPP_INCLUDED
#define _STRUS_WEIGHTING_EXECUTION_CONTEXT_INTERFACE_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/termStatistics.hpp"
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
	/// \param[in] name_ name of the summarization feature
	/// \param[in] postingIterator_ iterator on the matches of the weighting feature
	/// \param[in] weight_ weight of this feature
	/// \param[in] stats_ global term statistics passed down with the query. If undefined, they can be defined by or estimated from the posting iterator data. 
	/// \remark Do call this method before calling call the first time for not having incomplete results
	virtual void addWeightingFeature(
			const std::string& name_,
			PostingIteratorInterface* postingIterator_,
			float weight_,
			const TermStatistics& stats_)=0;

	/// \brief Call the weighting function for a document
	/// \param[in] docno document number
	/// \return the calculated weight of the document
	virtual float call( const Index& docno)=0;
};

}//namespace
#endif

