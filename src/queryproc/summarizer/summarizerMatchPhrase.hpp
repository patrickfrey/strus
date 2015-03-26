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
#ifndef _STRUS_SUMMARIZER_MATCH_PHRASE_HPP_INCLUDED
#define _STRUS_SUMMARIZER_MATCH_PHRASE_HPP_INCLUDED
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/summarizerClosureInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
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


class SummarizerClosureMatchPhrase
	:public SummarizerClosureInterface
{
public:
	/// \param[in] storage_ storage to use
	/// \param[in] processor_ query processor to use
	/// \param[in] termtype_ type of the tokens to build the summary with
	/// \param[in] maxlen_ maximum lenght of a sentence on both sides of the matching feature until it is cut and terminated with "..."
	/// \param[in] summarylen_ maximum lenght of the whole summary
	/// \param[in] features_ features to inspect
	SummarizerClosureMatchPhrase(
			const StorageClientInterface* storage_,
			const QueryProcessorInterface* processor_,
			const std::string& termtype_,
			unsigned int maxlen_,
			unsigned int summarylen_,
			const std::vector<SummarizerFunctionInterface::FeatureParameter>& features_);

	virtual ~SummarizerClosureMatchPhrase();

	/// \brief Get some summarization elements
	/// \param[in] docno document to get the summary element from
	/// \return the summarization elements
	virtual std::vector<SummaryElement> getSummary( const Index& docno);

private:
	const StorageClientInterface* m_storage;
	const QueryProcessorInterface* m_processor;
	Reference<ForwardIteratorInterface> m_forwardindex;
	std::string m_termtype;
	unsigned int m_maxlen;
	unsigned int m_summarylen;
	std::vector<PostingIteratorInterface*> m_itr;
	PostingIteratorInterface* m_phrasestruct;
	Reference<PostingIteratorInterface> m_structop;
};


class SummarizerFunctionMatchPhrase
	:public SummarizerFunctionInterface
{
public:
	SummarizerFunctionMatchPhrase(){}

	virtual ~SummarizerFunctionMatchPhrase(){}

	virtual const char** numericParameterNames() const
	{
		static const char* ar[] = {"phraselen","sumlen",0};
		return ar;
	}

	virtual const char** textualParameterNames() const
	{
		static const char* ar[] = {"type",0};
		return ar;
	}

	virtual const char** featureParameterClassNames() const
	{
		static const char* ar[] = {"struct","match",0};
		return ar;
	}
	static bool isStructFeature( std::size_t classidx)
	{
		return classidx == 0;
	}
	static bool isMatchFeature( std::size_t classidx)
	{
		return classidx == 1;
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
		unsigned int maxlen = numericParameters_[0].defined()?(unsigned int)numericParameters_[0]:30;
		unsigned int sumlen = numericParameters_[1].defined()?(unsigned int)numericParameters_[1]:40;
		if (termtype.empty())
		{
			throw strus::runtime_error( _TXT( "emtpy term type definition (parameter 'type') in match phrase summarizer configuration"));
		}
		return new SummarizerClosureMatchPhrase(
				storage_, processor_, termtype, maxlen, sumlen, features_);
	}
};

}//namespace
#endif


