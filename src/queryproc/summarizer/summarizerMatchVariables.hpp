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
#ifndef _STRUS_SUMMARIZER_MATCH_VARIABLES_HPP_INCLUDED
#define _STRUS_SUMMARIZER_MATCH_VARIABLES_HPP_INCLUDED
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/summarizerClosureInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/summarizationVariable.hpp"
#include "strus/reference.hpp"
#include "private/internationalization.hpp"
#include <vector>
#include <string>
#include <stdexcept>

namespace strus
{

/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class ForwardIteratorInterface;
/// \brief Forward declaration
class PostingIteratorInterface;
/// \brief Forward declaration
class QueryProcessorInterface;


class SummarizerClosureMatchVariables
	:public SummarizerClosureInterface
{
public:
	/// \param[in] storage_ storage to use
	/// \param[in] processor_ query processor to use
	/// \param[in] termtype_ type of the tokens to build the summary with
	/// \param[in] features_ features to inspect
	SummarizerClosureMatchVariables(
			const StorageClientInterface* storage_,
			const QueryProcessorInterface* processor_,
			const std::string& termtype_,
			const std::string& delimiter_,
			const std::string& assign_,
			const std::vector<SummarizerFunctionInterface::FeatureParameter>& features_);

	virtual ~SummarizerClosureMatchVariables();

	/// \brief Get some summarization elements
	/// \param[in] docno document to get the summary element from
	/// \return the summarization elements
	virtual std::vector<SummaryElement> getSummary( const Index& docno);

private:
	const StorageClientInterface* m_storage;
	const QueryProcessorInterface* m_processor;
	Reference<ForwardIteratorInterface> m_forwardindex;
	std::string m_termtype;
	std::string m_delimiter;
	std::string m_assign;
	std::vector<SummarizationFeature> m_features;
};


class SummarizerFunctionMatchVariables
	:public SummarizerFunctionInterface
{
public:
	SummarizerFunctionMatchVariables(){}

	virtual ~SummarizerFunctionMatchVariables(){}

	virtual const char** textualParameterNames() const
	{
		static const char* ar[] = {"type",0};
		return ar;
	}

	virtual const char** featureParameterClassNames() const
	{
		static const char* ar[] = {"match","delimiter","assign",0};
		return ar;
	}

	virtual SummarizerClosureInterface* createClosure(
			const StorageClientInterface* storage_,
			const QueryProcessorInterface* processor_,
			MetaDataReaderInterface* metadata_,
			const std::vector<FeatureParameter>& features_,
			const std::vector<std::string>& textualParameters_,
			const std::vector<ArithmeticVariant>& numericParameters_) const
	{
		std::string termtype = textualParameters_[0];
		std::string delimiter = textualParameters_[1];
		std::string assign = textualParameters_[2];
		if (termtype.empty())
		{
			throw strus::runtime_error( _TXT( "empty term type definition (parameter 'type') in match variables summarizer configuration"));
		}
		return new SummarizerClosureMatchVariables(
				storage_, processor_, termtype, delimiter, assign, features_);
	}
};

}//namespace
#endif


