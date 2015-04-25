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
#include "strus/summarizerClosureInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/reference.hpp"
#include "private/internationalization.hpp"
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


class SummarizerClosureMatchPhrase
	:public SummarizerClosureInterface
{
public:
	/// \param[in] storage_ storage to use
	/// \param[in] processor_ query processor to use
	/// \param[in] type_ type of the tokens to build the summary with
	/// \param[in] maxlen_ maximum lenght of a sentence on both sides of the matching feature until it is cut and terminated with "..."
	/// \param[in] summarylen_ maximum lenght of the whole summary
	/// \param[in] features_ features to inspect
	SummarizerClosureMatchPhrase(
			const StorageClientInterface* storage_,
			const QueryProcessorInterface* processor_,
			const std::string& type_,
			unsigned int maxlen_,
			unsigned int summarylen_);
	virtual ~SummarizerClosureMatchPhrase();

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
	unsigned int m_maxlen;
	unsigned int m_summarylen;
	std::vector<PostingIteratorInterface*> m_itr;
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
	explicit SummarizerFunctionInstanceMatchPhrase()
		:m_type(),m_maxlen(30),m_sumlen(40){}

	virtual ~SummarizerFunctionInstanceMatchPhrase(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const ArithmeticVariant& value);

	virtual SummarizerClosureInterface* createClosure(
			const StorageClientInterface* storage,
			const QueryProcessorInterface* processor,
			MetaDataReaderInterface*) const
	{
		if (m_type.empty())
		{
			throw strus::runtime_error( _TXT( "emtpy term type definition (parameter 'type') in match phrase summarizer configuration"));
		}
		return new SummarizerClosureMatchPhrase(
				storage, processor, m_type, m_maxlen, m_sumlen);
	}

	virtual std::string tostring() const
	{
		std::ostringstream rt;
		rt << "type='" << m_type 
			<< "', phraselen=" << m_maxlen
			<< ", sumlen=" << m_sumlen;
		return rt.str();
	}

private:
	std::string m_type;
	unsigned int m_maxlen;
	unsigned int m_sumlen;
};


class SummarizerFunctionMatchPhrase
	:public SummarizerFunctionInterface
{
public:
	SummarizerFunctionMatchPhrase(){}

	virtual ~SummarizerFunctionMatchPhrase(){}

	virtual SummarizerFunctionInstanceInterface* createInstance() const
	{
		return new SummarizerFunctionInstanceMatchPhrase();
	}
};

}//namespace
#endif


