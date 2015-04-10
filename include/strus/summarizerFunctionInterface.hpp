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
#ifndef _STRUS_SUMMARIZER_FUNCTION_INTERFACE_HPP_INCLUDED
#define _STRUS_SUMMARIZER_FUNCTION_INTERFACE_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/arithmeticVariant.hpp"
#include "strus/summarizationFeature.hpp"
#include <vector>

namespace strus
{
/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class QueryProcessorInterface;
/// \brief Forward declaration
class PostingIteratorInterface;
/// \brief Forward declaration
class ForwardIteratorInterface;
/// \brief Forward declaration
class MetaDataReaderInterface;
/// \brief Forward declaration
class SummarizerClosureInterface;


/// \brief Interface for summarization (additional info about the matches in the result ranklist of a retrieval query)
class SummarizerFunctionInterface
{
public:
	virtual ~SummarizerFunctionInterface(){}

	/// \brief Get the numeric value parameter names of the function in the order they should be passed
	/// \return the NULL terminated list of parameter names or NULL, if there are none
	virtual const char** numericParameterNames() const		{return 0;}

	/// \brief Get the textual value parameter names of the function in the order they should be passed
	/// \return the NULL terminated list of parameter names or NULL, if there are none
	virtual const char** textualParameterNames() const		{return 0;}

	/// \brief Get the feature class names of the function that define the class of the feature. The index of the name in this array (starting with 0) is assigned to the posting iterator when passing it to the function.
	/// \return the NULL terminated list of feature class names or NULL, if there are none
	virtual const char** featureParameterClassNames() const		{return 0;}

	/// \brief Definition of an argument feature for summarization
	class FeatureParameter
	{
	public:
		FeatureParameter( unsigned int classidx_, const SummarizationFeature& feature_)
			:m_classidx(classidx_),m_feature(feature_){}
		FeatureParameter( const FeatureParameter& o)
			:m_classidx(o.m_classidx),m_feature(o.m_feature){}

		unsigned int classidx() const				{return m_classidx;}
		const SummarizationFeature& feature() const		{return m_feature;}

	private:
		unsigned int m_classidx;
		SummarizationFeature m_feature;
	};

	/// \brief Create a closure (execution context) for this summarization function
	/// \param[in] storage_ storage interface for getting information for summarization (like for example document attributes)
	/// \param[in] processor_ query processor to get posting set operators of weighting functions from
	/// \param[in] metadata_ metadata interface for getting information for summarization (like for example the document insertion date)
	/// \param[in] features_ features that are subject of summarization (like for example query features for extracting matching phrases)
	/// \param[in] textualParameters_ parameters for summarization as strings
	/// \param[in] numericParameters_ numeric parameters for summarization
	/// \return the closure (the summarization function with its execution context)
	virtual SummarizerClosureInterface* createClosure(
			const StorageClientInterface* storage_,
			const QueryProcessorInterface* processor_,
			MetaDataReaderInterface* metadata_,
			const std::vector<FeatureParameter>& features_,
			const std::vector<std::string>& textualParameters_,
			const std::vector<ArithmeticVariant>& numericParameters_) const=0;
};

}//namespace
#endif


