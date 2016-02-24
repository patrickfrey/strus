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

class SummarizerFunctionContextMatchPhrase
	:public SummarizerFunctionContextInterface
{
public:
	/// \param[in] storage_ storage to use
	/// \param[in] processor_ query processor to use
	/// \param[in] metadata_ meta data reader
	/// \param[in] type_ type of the tokens to build the summary with
	/// \param[in] sentencesize_ maximum lenght of a sentence until it is cut with "..."
	/// \param[in] windowsize_ maximum size of window to look for matches
	/// \param[in] cardinality_ minimum number of features to look for in a window
	/// \param[in] nofCollectionDocuments_ number of documents in the collection
	/// \param[in] metadata_title_maxpos_ optional attribute in metadata that defines the last position in the document that belongs to the document title and should not be considered for summary
	/// \param[in] matchmark_ begin and end marker for highlighting
	/// \param[in] floatingmark_ begin and end marker for not terminated sentences
	/// \param[in] name_para_ name of summary elements defining paragraphs
	/// \param[in] name_phrase_ name of summary elements defining phrases
	/// \param[in] errorhnd_ error buffer interface
	SummarizerFunctionContextMatchPhrase(
			const StorageClientInterface* storage_,
			const QueryProcessorInterface* processor_,
			MetaDataReaderInterface* metadata_,
			const std::string& type_,
			unsigned int sentencesize_,
			unsigned int windowsize_,
			unsigned int cardinality_,
			double nofCollectionDocuments_,
			const std::string& metadata_title_maxpos_,
			const std::pair<std::string,std::string>& matchmark_,
			const std::pair<std::string,std::string>& floatingmark_,
			const std::string& name_para_,
			const std::string& name_phrase_,
			ErrorBufferInterface* errorhnd_);
	virtual ~SummarizerFunctionContextMatchPhrase();

	virtual void addSummarizationFeature(
			const std::string& name,
			PostingIteratorInterface* itr,
			const std::vector<SummarizationVariable>&,
			float weight,
			const TermStatistics&);

	virtual std::vector<SummaryElement> getSummary( const Index& docno);

private:
	enum {MaxNofArguments=64};				///< chosen to fit in a bitfield of 64 bits
	const StorageClientInterface* m_storage;		///< storage access
	const QueryProcessorInterface* m_processor;		///< query processor interface
	MetaDataReaderInterface* m_metadata;			///< access metadata arguments
	Reference<ForwardIteratorInterface> m_forwardindex;	///< forward index iterator
	std::string m_type;					///< forward index type to extract
	unsigned int m_sentencesize;				///< search area for end of sentence
	unsigned int m_windowsize;				///< maximum window size
	unsigned int m_cardinality;				///< window cardinality
	double m_nofCollectionDocuments;			///< number of documents in the collection
	int m_metadata_title_maxpos;				///< meta data element for maximum title position
	std::pair<std::string,std::string> m_matchmark;		///< highlighting info
	std::pair<std::string,std::string> m_floatingmark;	///< marker for unterminated begin and end phrase
	std::string m_name_para;				///< name of the summary elements for paragraphs
	std::string m_name_phrase;				///< name of the summary elements for phrases
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
		:m_type(),m_sentencesize(100),m_windowsize(100),m_cardinality(0)
		,m_matchmark()
		,m_floatingmark(std::pair<std::string,std::string>("... "," ..."))
		,m_name_para("para")
		,m_name_phrase("phrase")
		,m_processor(processor_),m_errorhnd(errorhnd_){}

	virtual ~SummarizerFunctionInstanceMatchPhrase(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const ArithmeticVariant& value);

	virtual SummarizerFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage,
			MetaDataReaderInterface*,
			const GlobalStatistics&) const;

	virtual std::string tostring() const;

private:
	std::string m_type;					///< forward index type to extract
	std::string m_metadata_title_maxpos;			///< name of metadata element for the last title element position 
	unsigned int m_sentencesize;				///< search area for end of sentence
	unsigned int m_windowsize;				///< maximum window size
	unsigned int m_cardinality;				///< window cardinality
	std::pair<std::string,std::string> m_matchmark;		///< highlight marker for matches
	std::pair<std::string,std::string> m_floatingmark;	///< marker for unterminated begin and end phrase
	std::string m_name_para;				///< name of the summary elements for paragraphs
	std::string m_name_phrase;				///< name of the summary elements for phrases
	const QueryProcessorInterface* m_processor;		///< query processor interface
	ErrorBufferInterface* m_errorhnd;			///< buffer for error messages
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


