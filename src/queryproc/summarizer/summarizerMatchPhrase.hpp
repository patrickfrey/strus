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
class MetaDataReaderInterface;
/// \brief Forward declaration
class ForwardIteratorInterface;
/// \brief Forward declaration
class QueryProcessorInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

enum {
	MaxParaTitleSize=12
};

/// \brief Configured parameters of the MatchPhrase summarizer function
struct SummarizerFunctionParameterMatchPhrase
{
	typedef ProximityWeightingContext::Config ProximityWeightingConfig;
	ProximityWeightingConfig proximityConfig;	///< configuration for proximity weighting
	std::string name;				///< name of the summary element
	std::string textType;				///< name of the forward index feature to collect as text
	std::string wordType;				///< name of the forward index feature that contains the word type of the feature to collect in the forward index; must align with the entity type and defines the length, resp. the number of terms overed by the entity
	std::string entityType;				///< name of the forward index feature that contains the entity assigned to the aligned text feature

	SummarizerFunctionParameterMatchPhrase()
		:proximityConfig(),name(),textType(),wordType(),entityType()
	{}
	SummarizerFunctionParameterMatchPhrase( const SummarizerFunctionParameterMatchPhrase& o)
		:proximityConfig(o.proximityConfig)
		,name(o.name),textType(o.textType),wordType(o.wordType),entityType(o.entityType)
	{}

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
			const QueryProcessorInterface* processor_,
			MetaDataReaderInterface* metadata_,
			const Reference<SummarizerFunctionParameterMatchPhrase>& parameter_,
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

	virtual std::string debugCall( const strus::WeightedDocument& doc);

public:
	enum {MaxNofArguments=64};				///< chosen to fit in a bitfield of 64 bits

private:
	ProximityWeightingContext m_proximityWeightingContext;		///< proximity weighting context
	SummarizerFunctionParameterMatchPhrase m_parameter;		///< parameter
	PostingIteratorInterface* m_itrar[ MaxNofArguments];		///< posting iterators to weight
	std::size_t m_itrarsize;					///< nof posting iterators defined to weight
	FeatureWeights m_weightar;					///< array of feature weights parallel to m_itrar
	PostingIteratorInterface* m_eos_itr;				///< posting iterators for end of sentence markers
	double m_nofCollectionDocuments;				///< number of documents in the collection
	const StorageClientInterface* m_storage;			///< storage client interface
	ErrorBufferInterface* m_errorhnd;				///< buffer for error reporting

private:

	double windowWeight( WeightingData& wdata, const PositionWindow& poswin, const strus::IndexRange& structframe, const strus::IndexRange& paraframe);

	void fetchNoTitlePostings( WeightingData& wdata, PostingIteratorInterface** itrar, Index& cntTitleTerms, Index& cntNoTitleTerms);
	Match findBestMatch_( WeightingData& wdata, unsigned int cardinality, PostingIteratorInterface** itrar);
	Match findBestMatch( WeightingData& wdata);
	Match findBestMatchNoTitle( WeightingData& wdata);
	Match findAbstractMatch( WeightingData& wdata);

	Match logFindBestMatch_( std::ostream& out, WeightingData& wdata, unsigned int cardinality, PostingIteratorInterface** itrar);
	Match logFindBestMatchNoTitle( std::ostream& out, WeightingData& wdata);
	Match logFindAbstractMatch( std::ostream& out, WeightingData& wdata);

	Abstract getPhraseAbstract( const Match& candidate, WeightingData& wdata);
	Abstract getParaTitleAbstract( Match& phrase_match, WeightingData& wdata);

	std::string getParaTitleString( const Abstract& para_abstract);
	std::string getPhraseString( const Abstract& phrase_abstract, WeightingData& wdata);
	std::string getPhraseString( strus::Index firstpos, strus::Index lastpos);

	std::vector<SummaryElement>
		getSummariesFromAbstracts(
			const Abstract& para_abstract,
			const Abstract& phrase_abstract,
			WeightingData& wdata);

private:
	const StorageClientInterface* m_storage;		///< storage access
	const QueryProcessorInterface* m_processor;		///< query processor interface
	Reference<MetaDataReaderInterface> m_metadata;		///< access metadata arguments
	Reference<ForwardIteratorInterface> m_forwardindex;	///< forward index iterator
	Reference<SummarizerFunctionParameterMatchPhrase> m_parameter;
	double m_nofCollectionDocuments;			///< number of documents in the collection
	ProximityWeightAccumulator::WeightArray m_idfar;	///< array of idfs
	PostingIteratorInterface* m_itrar[ MaxNofArguments];	///< array if weighted features
	PostingIteratorInterface* m_structar[ MaxNofArguments];	///< array of end of structure elements
	std::size_t m_itrarsize;				///< number of weighted features
	std::size_t m_structarsize;				///< number of end of structure elements
	std::size_t m_paraarsize;				///< number of paragraph elements (now summary accross paragraph borders)
	std::size_t m_nof_maxdf_features;			///< number of features with a df bigger than maximum
	unsigned int m_cardinality;				///< calculated cardinality
	ProximityWeightAccumulator::WeightArray m_weightincr;	///< array of proportional weight increments 
	bool m_initialized;					///< true, if the structures have already been initialized
	PostingIteratorInterface* m_titleitr;			///< iterator to identify the title field for weight increment
	ErrorBufferInterface* m_errorhnd;			///< buffer for error messages
};


/// \class SummarizerFunctionInstanceMatchPhrase
/// \brief Summarizer instance for retrieving meta data
class SummarizerFunctionInstanceMatchPhrase
	:public SummarizerFunctionInstanceInterface
{
public:
	SummarizerFunctionInstanceMatchPhrase( const QueryProcessorInterface* processor_, ErrorBufferInterface* errorhnd_)
		:m_parameter( new SummarizerFunctionParameterMatchPhrase())
		,m_processor(processor_)
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

	virtual const char* name() const {return "matchphrase";}
	virtual StructView view() const;

private:
	Reference<SummarizerFunctionParameterMatchPhrase> m_parameter;
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

	virtual const char* name() const {return "matchphrase";}
	virtual StructView view() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif


