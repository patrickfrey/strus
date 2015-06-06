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
#include "strus/summarizerFunctionInstanceInterface.hpp"
#include "strus/summarizerExecutionContextInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/reference.hpp"
#include "private/internationalization.hpp"
#include "slidingMatchWindow.hpp"
#include <vector>
#include <string>
#include <stdexcept>
#include <sstream>
#include <iostream>

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


class SummarizerExecutionContextMatchPhrase
	:public SummarizerExecutionContextInterface
{
public:
	/// \param[in] storage_ storage to use
	/// \param[in] processor_ query processor to use
	/// \param[in] type_ type of the tokens to build the summary with
	/// \param[in] maxlen_ maximum lenght of a sentence on both sides of the matching feature until it is cut and terminated with "..."
	/// \param[in] summarylen_ maximum lenght of the whole summary
	/// \param[in] features_ features to inspect
	SummarizerExecutionContextMatchPhrase(
			const StorageClientInterface* storage_,
			const QueryProcessorInterface* processor_,
			const std::string& type_,
			unsigned int maxlen_,
			unsigned int summarylen_,
			unsigned int structseeklen_,
			const std::pair<std::string,std::string>& matchmark_);
	virtual ~SummarizerExecutionContextMatchPhrase();

	virtual void addSummarizationFeature(
			const std::string& name,
			PostingIteratorInterface* itr,
			const std::vector<SummarizationVariable>&);

	virtual std::vector<SummaryElement> getSummary( const Index& docno);

private:
	const StorageClientInterface* m_storage;
	const QueryProcessorInterface* m_processor;
	Reference<ForwardIteratorInterface> m_forwardindex;
	std::string m_type;
	unsigned int m_nofsummaries;
	unsigned int m_summarylen;
	unsigned int m_structseeklen;
	std::pair<std::string,std::string> m_matchmark;
	float m_nofCollectionDocuments;
	std::vector<PostingIteratorInterface*> m_itr;
	std::vector<float> m_weights;
	PostingIteratorInterface* m_phrasestruct;
	Reference<PostingIteratorInterface> m_structop;
	std::vector<Reference<PostingIteratorInterface> > m_structelem;
	bool m_init_complete;
};


/// \class SummarizerFunctionInstanceMatchPhrase
/// \brief Summarizer instance for retrieving meta data
class SummarizerFunctionInstanceMatchPhrase
	:public SummarizerFunctionInstanceInterface
{
public:
	explicit SummarizerFunctionInstanceMatchPhrase( const QueryProcessorInterface* processor)
		:m_type(),m_nofsummaries(3),m_summarylen(40),m_structseeklen(10),m_processor(processor){}

	virtual ~SummarizerFunctionInstanceMatchPhrase(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const ArithmeticVariant& value);

	virtual SummarizerExecutionContextInterface* createExecutionContext(
			const StorageClientInterface* storage,
			MetaDataReaderInterface*) const
	{
		if (m_type.empty())
		{
			throw strus::runtime_error( _TXT( "emtpy term type definition (parameter 'type') in match phrase summarizer configuration"));
		}
		return new SummarizerExecutionContextMatchPhrase(
				storage, m_processor, m_type, m_nofsummaries, m_summarylen, m_structseeklen, m_matchmark);
	}

	virtual std::string tostring() const
	{
		std::ostringstream rt;
		rt << "type='" << m_type 
			<< "', nof summaries=" << m_nofsummaries
			<< ", summarylen=" << m_summarylen;
		return rt.str();
	}

private:
	std::string m_type;
	unsigned int m_nofsummaries;
	unsigned int m_summarylen;
	unsigned int m_structseeklen;
	std::pair<std::string,std::string> m_matchmark;
	const QueryProcessorInterface* m_processor;
};


class SummarizerFunctionMatchPhrase
	:public SummarizerFunctionInterface
{
public:
	SummarizerFunctionMatchPhrase(){}

	virtual ~SummarizerFunctionMatchPhrase(){}

	virtual SummarizerFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface* processor) const
	{
		return new SummarizerFunctionInstanceMatchPhrase( processor);
	}
};

}//namespace
#endif


