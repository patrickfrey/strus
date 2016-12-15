/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
	/// \param[in] maxdf_ fraction of collection size, defining what is considered to be a stopword or not
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
			double maxdf_,
			const std::pair<std::string,std::string>& matchmark_,
			const std::pair<std::string,std::string>& floatingmark_,
			const std::string& name_para_,
			const std::string& name_phrase_,
			const std::string& name_docstart_,
			ErrorBufferInterface* errorhnd_);
	virtual ~SummarizerFunctionContextMatchPhrase();

	virtual void addSummarizationFeature(
			const std::string& name,
			PostingIteratorInterface* itr,
			const std::vector<SummarizationVariable>&,
			double weight,
			const TermStatistics&);

	virtual std::vector<SummaryElement> getSummary( const Index& docno);

public:
	enum {MaxNofArguments=64};				///< chosen to fit in a bitfield of 64 bits

private:
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
	double m_maxdf;						///< the maximum df of features considered for same sentence proximity weighing as fraction of the total collection size
	std::pair<std::string,std::string> m_matchmark;		///< highlighting info
	std::pair<std::string,std::string> m_floatingmark;	///< marker for unterminated begin and end phrase
	std::string m_name_para;				///< name of the summary elements for paragraphs
	std::string m_name_phrase;				///< name of the summary elements for phrases
	std::string m_name_docstart;				///< name of the summary elements for document start (alternative summary if no match found)
	ProximityWeightAccumulator::WeightArray m_idfar;	///< array of idfs
	PostingIteratorInterface* m_itrar[ MaxNofArguments];	///< array if weighted features
	PostingIteratorInterface* m_structar[ MaxNofArguments];	///< array of end of structure elements
	PostingIteratorInterface* m_paraar[ MaxNofArguments];	///< array of end of paragraph elements
	std::size_t m_itrarsize;				///< number of weighted features
	std::size_t m_structarsize;				///< number of end of structure elements
	std::size_t m_paraarsize;				///< number of paragraph elements (now summary accross paragraph borders)
	Index m_maxdist_featar[ MaxNofArguments];		///< array of distances indicating what proximity distance is considered at maximum for same sentence weight
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
		,m_name_docstart("docstart")
		,m_processor(processor_)
		,m_errorhnd(errorhnd_){}

	virtual ~SummarizerFunctionInstanceMatchPhrase(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const NumericVariant& value);

	virtual void defineResultName(
			const std::string& resultname,
			const std::string& itemname);

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
	double m_maxdf;						///< df limit for judging if a term is a stopword or not
	std::pair<std::string,std::string> m_matchmark;		///< highlight marker for matches
	std::pair<std::string,std::string> m_floatingmark;	///< marker for unterminated begin and end phrase
	std::string m_name_para;				///< name of the summary elements for paragraphs
	std::string m_name_phrase;				///< name of the summary elements for phrases
	std::string m_name_docstart;				///< name of the summary elements for document start (alternative summary if no match found)
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

	virtual FunctionDescription getDescription() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif


