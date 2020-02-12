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
#include "proximityWeightingContext.hpp"
#include "forwardIndexTextCollector.hpp"
#include "sentenceIterator.hpp"
#include <vector>
#include <string>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <limits>

namespace strus
{

/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class ForwardIteratorInterface;
/// \brief Forward declaration
class QueryProcessorInterface;
/// \brief Forward declaration
class ErrorBufferInterface;


/// \brief Configured parameters of the MatchPhrase summarizer function
struct SummarizerFunctionParameterMatchPhrase
{
	typedef ProximityWeightingContext::Config ProximityWeightingConfig;
	ProximityWeightingConfig proximityConfig;	///< configuration for proximity weighting
	std::string contentType;			///< name of the forward index feature to collect as text
	std::string wordType;				///< name of the forward index feature that contains the word type of the feature to collect in the forward index; must align with the entity type and defines the length, resp. the number of terms overed by the entity
	std::string entityType;				///< name of the forward index feature that contains the entity assigned to the aligned text feature
	double maxdf;					///< the maximum df of features not to be considered stopwords as fraction of the total collection size

	SummarizerFunctionParameterMatchPhrase()
		:proximityConfig(),contentType(),wordType(),entityType()
	{}
	SummarizerFunctionParameterMatchPhrase( const SummarizerFunctionParameterMatchPhrase& o)
		:proximityConfig(o.proximityConfig)
		,contentType(o.contentType),wordType(o.wordType),entityType(o.entityType)
	{}
};

class SummarizerFunctionContextMatchPhrase
	:public SummarizerFunctionContextInterface
{
public:
	/// \param[in] storage_ storage to use
	/// \param[in] processor_ query processor to use
	/// \param[in] metadata_ meta data reader
	/// \param[in] parameter_ parameter for weighting
	/// \param[in] nofCollectionDocuments_ number of documents in the collection
	/// \param[in] errorhnd_ error buffer interface
	SummarizerFunctionContextMatchPhrase(
			const StorageClientInterface* storage_,
			const SummarizerFunctionParameterMatchPhrase& parameter_,
			double nofCollectionDocuments_,
			ErrorBufferInterface* errorhnd_);
	virtual ~SummarizerFunctionContextMatchPhrase();

	virtual void addSummarizationFeature(
			const std::string& name_,
			PostingIteratorInterface* itr,
			const std::vector<SummarizationVariable>&,
			double weight,
			const TermStatistics&);

	virtual void setVariableValue( const std::string& name_, double value);

	virtual std::vector<SummaryElement> getSummary( const strus::WeightedDocument& doc);

private:
	enum {MaxNofArguments=ProximityWeightingContext::MaxNofArguments};	///< maximum number of arguments fix because of extensive use of fixed size arrays
	typedef ProximityWeightingContext::FeatureWeights FeatureWeights;

private:
	ProximityWeightingContext m_proximityWeightingContext;		///< proximity weighting context
	SummarizerFunctionParameterMatchPhrase m_parameter;		///< parameter
	PostingIteratorInterface* m_itrar[ MaxNofArguments];		///< posting iterators to weight
	std::size_t m_itrarsize;					///< nof posting iterators defined to weight
	FeatureWeights m_weightar;					///< array of feature weights parallel to m_itrar
	PostingIteratorInterface* m_eos_itr;				///< posting iterators for end of sentence markers
	double m_nofCollectionDocuments;				///< number of documents in the collection
	const StorageClientInterface* m_storage;			///< storage client interface
	ForwardIndexTextCollector m_textCollector;
	ErrorBufferInterface* m_errorhnd;				///< buffer for error reporting
};


/// \class SummarizerFunctionInstanceMatchPhrase
/// \brief Summarizer instance for retrieving meta data
class SummarizerFunctionInstanceMatchPhrase
	:public SummarizerFunctionInstanceInterface
{
public:
	explicit SummarizerFunctionInstanceMatchPhrase( ErrorBufferInterface* errorhnd_)
		:m_parameter()
		,m_errorhnd(errorhnd_){}

	virtual ~SummarizerFunctionInstanceMatchPhrase(){}

	virtual void addStringParameter( const std::string& name_, const std::string& value);
	virtual void addNumericParameter( const std::string& name_, const NumericVariant& value);

	virtual std::vector<std::string> getVariables() const
	{
		return std::vector<std::string>();
	}

	virtual SummarizerFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage,
			const GlobalStatistics&) const;

	virtual const char* name() const;
	virtual StructView view() const;

private:
	SummarizerFunctionParameterMatchPhrase m_parameter;	///< configured parameters
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

	virtual const char* name() const;
	virtual StructView view() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif


