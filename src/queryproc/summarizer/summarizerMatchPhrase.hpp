/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
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
#include "strus/summarizerFunctionContextInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/reference.hpp"
#include "private/internationalization.hpp"
#include "proximityWeightAccumulator.hpp"
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
class MetaDataReaderInterface;
/// \brief Forward declaration
class ForwardIteratorInterface;
/// \brief Forward declaration
class QueryProcessorInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

/*
Use PositionWindow to iterate on Windows that do not overlap with the title:
For each window assign to each feature the idf times a measure,
          that is taken from a table between 70% and 100% depending on the abs(log(position))
          from the start of the document. 
For each feature multiply its weight with values between 1.3 to 1.0 depending on neighborhood of other features
          with criteria immediately followed, immediately followed also in the query, close (dist < 5),
          in the same sentence
Add the calculate measures together. Because the weight factors have an upper bound we can prune calculation,
          e.g. if the sum of idfs is below factor 2.
The window with the highest weight wins and all the sentences (without title) that are overlaped by the window
          form the summary.
*/

class SummarizerFunctionContextMatchPhrase
	:public SummarizerFunctionContextInterface
{
public:
	/// \param[in] storage_ storage to use
	/// \param[in] processor_ query processor to use
	/// \param[in] type_ type of the tokens to build the summary with
	/// \param[in] sentencesize_ maximum lenght of a sentence until it is cut with "..."
	/// \param[in] windowsize_ maximum size of window to look for matches
	/// \param[in] cardinality_ minimum number of features to look for in a window
	/// \param[in] nofCollectionDocuments_ number of documents in the collection
	/// \param[in] attribute_header_maxpos_ optional attribute in metadata that defines the last position in the document that belongs to the document header and should not be considered for summary
	/// \param[in] matchmark_ begin and marker for highlighting
	/// \param[in] errorhnd_ error buffer interface
	SummarizerFunctionContextMatchPhrase(
			const StorageClientInterface* storage_,
			const MetaDataReaderInterface* metadata_,
			const QueryProcessorInterface* processor_,
			const std::string& type_,
			unsigned int sentencesize_,
			unsigned int windowsize_,
			unsigned int cardinality_,
			double nofCollectionDocuments_,
			const std::string& attribute_header_maxpos_,
			const std::pair<std::string,std::string>& matchmark_,
			ErrorBufferInterface* errorhnd_);
	virtual ~SummarizerFunctionContextMatchPhrase();

	virtual void addSummarizationFeature(
			const std::string& name,
			PostingIteratorInterface* itr,
			const std::vector<SummarizationVariable>&,
			float /*weight*/,
			const TermStatistics&);

	virtual std::vector<SummaryElement> getSummary( const Index& docno);

private:
	enum {MaxNofArguments=64};
	const StorageClientInterface* m_storage;
	const MetaDataReaderInterface* m_metadata;
	const QueryProcessorInterface* m_processor;
	Reference<ForwardIteratorInterface> m_forwardindex;	///< forward index iterator
	std::string m_type;					///< forward index type
	unsigned int m_sentencesize;				///< search area for end of sentence
	unsigned int m_windowsize;				///< maximum window size
	unsigned int m_cardinality;				///< window cardinality
	double m_nofCollectionDocuments;			///< number of documents in the collection
	int m_metadata_header_maxpos;				///< meta data element for maximum title position
	std::pair<std::string,std::string> m_matchmark;		///< highlighting info
	ProximityWeightAccumulator::WeightArray m_idfar;	///< array of idfs
	PostingIteratorInterface* m_itrar[ MaxNofArguments];	///< array if weighted features
	PostingIteratorInterface* m_structar[ MaxNofArguments];	///< array of end of structure elements
	PostingIteratorInterface* m_paraar[ MaxNofArguments];	///< array of end of paragraph elements
	std::size_t m_itrarsize;				///< number of weighted features
	std::size_t m_structarsize;				///< number of end of structure elements
	std::size_t m_paraarsize;				///< number of paragraph elements (now summary accross paragraph borders)
	ProximityWeightAccumulator::WeightArray m_weightincr;	///< array of proportional weight increments 
	bool m_initialized;					///< true, if the structures have already been initialized
	ErrorBufferInterface* m_errorhnd;			///< buffer for error messages
};


/// \class SummarizerFunctionInstanceMatchPhrase
/// \brief Summarizer instance for retrieving meta data
class SummarizerFunctionInstanceMatchPhrase
	:public SummarizerFunctionInstanceInterface
{
public:
	SummarizerFunctionInstanceMatchPhrase( const QueryProcessorInterface* processor_, ErrorBufferInterface* errorhnd_)
		:m_type(),m_sentencesize(100),m_windowsize(100),m_cardinality(0),m_processor(processor_),m_errorhnd(errorhnd_){}

	virtual ~SummarizerFunctionInstanceMatchPhrase(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const ArithmeticVariant& value);

	virtual SummarizerFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage,
			MetaDataReaderInterface*,
			const GlobalStatistics&) const;

	virtual std::string tostring() const;

private:
	std::string m_type;
	std::string m_attribute_title_maxpos;
	unsigned int m_sentencesize;
	unsigned int m_windowsize;
	unsigned int m_cardinality;
	std::pair<std::string,std::string> m_matchmark;
	const QueryProcessorInterface* m_processor;
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


class SummarizerFunctionMatchPhrase
	:public SummarizerFunctionInterface
{
public:
	explicit SummarizerFunctionMatchPhrase( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}

	virtual ~SummarizerFunctionMatchPhrase(){}

	virtual SummarizerFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface* processor) const;

	virtual Description getDescription() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif


