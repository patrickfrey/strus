/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_SUMMARIZER_ACCUMULATE_NEAR_HPP_INCLUDED
#define _STRUS_SUMMARIZER_ACCUMULATE_NEAR_HPP_INCLUDED
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/summarizerFunctionInstanceInterface.hpp"
#include "strus/summarizerFunctionContextInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/storage/summarizationVariable.hpp"
#include "strus/reference.hpp"
#include "proximityWeightingContext.hpp"
#include "forwardIndexCollector.hpp"
#include <vector>
#include <string>
#include <map>
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
/// \brief Forward declaration
class ErrorBufferInterface;

struct SummarizerFunctionParameterAccumulateNear
{
	typedef ProximityWeightingContext::Config ProximityWeightingConfig;
	struct CollectorConfig
	{
		std::string name;
		std::vector<std::string> collectTypes;
		std::string tagType;
		std::string stripCharacters;
		char tagSeparator;

		CollectorConfig( const std::string& name_, const std::vector<std::string>& collectTypes_, const std::string& tagType_, const std::string& stripCharacters_, char tagSeparator_)
			:name(name_),collectTypes(collectTypes_),tagType(tagType_),stripCharacters(stripCharacters_),tagSeparator(tagSeparator_){}
		CollectorConfig( const CollectorConfig& o)
			:name(o.name),collectTypes(o.collectTypes),tagType(o.tagType),stripCharacters(o.stripCharacters),tagSeparator(o.tagSeparator){}

		StructView view() const;
	};

	ProximityWeightingConfig proximityConfig;		///< configuration for proximity weighting
	int maxNofResults;					///< maximum number of weighted features returned
	int distance_collect;					///< distance to matches to collect
	double maxdf;						///< the maximum df of features not to be considered stopwords as fraction of the total collection size
	std::vector<CollectorConfig> collectorConfigs;		///< Grouped features from forward index to collect

	SummarizerFunctionParameterAccumulateNear()
		:proximityConfig()
		,maxNofResults(20)
		,distance_collect(8)
		,maxdf(0.5)
		,collectorConfigs()
	{}
	SummarizerFunctionParameterAccumulateNear( const SummarizerFunctionParameterAccumulateNear& o)
		:proximityConfig(o.proximityConfig)
		,maxNofResults(o.maxNofResults)
		,distance_collect(o.distance_collect)
		,maxdf(o.maxdf)
		,collectorConfigs(o.collectorConfigs)
	{}

	void addConfig( const std::string& configstr, ErrorBufferInterface* errorhnd);
};

class SummarizerFunctionContextAccumulateNear
	:public SummarizerFunctionContextInterface
{
public:
	SummarizerFunctionContextAccumulateNear(
			const StorageClientInterface* storage_,
			const SummarizerFunctionParameterAccumulateNear& parameter_,
			double nofCollectionDocuments_,
			ErrorBufferInterface* errorhnd_);

	virtual ~SummarizerFunctionContextAccumulateNear(){}

	virtual void addSummarizationFeature(
			const std::string& name,
			PostingIteratorInterface* itr,
			const std::vector<SummarizationVariable>& variables,
			double weight);

	virtual void setVariableValue( const std::string& name, double value);

	virtual std::vector<SummaryElement> getSummary( const strus::WeightedDocument& doc);

private:
	enum {MaxNofArguments=ProximityWeightingContext::MaxNofArguments};	///< maximum number of arguments fix because of extensive use of fixed size arrays
	typedef ProximityWeightingContext::FeatureWeights FeatureWeights;
	typedef ProximityWeightingContext::WeightedNeighbour WeightedNeighbour;
	
private:
	void initializeContext();

	typedef std::map<std::string,double> EntityMap;
	void collectSummariesFromEntityMap( std::vector<SummaryElement>& res, const std::string& name, EntityMap& emap) const;

private:
	ProximityWeightingContext m_proximityWeightingContext;		///< proximity weighting context
	SummarizerFunctionParameterAccumulateNear m_parameter;		///< parameter
	PostingIteratorInterface* m_itrar[ MaxNofArguments];		///< posting iterators to weight
	std::size_t m_itrarsize;					///< nof posting iterators defined to weight
	FeatureWeights m_weightar;					///< array of feature weights parallel to m_itrar
	PostingIteratorInterface* m_stopword_itrar[ MaxNofArguments];	///< posting iterators to weight
	std::size_t m_stopword_itrarsize;				///< nof posting iterators defined to weight
	FeatureWeights m_stopword_weightar;				///< array of feature weights parallel to m_itrar
	PostingIteratorInterface* m_eos_itr;				///< posting iterators for end of sentence markers
	double m_nofCollectionDocuments;				///< number of documents in the collection
	const StorageClientInterface* m_storage;			///< storage client interface
	std::vector<ForwardIndexCollector> m_collectors;		///< collector list
	double m_weightnorm;						///< normalization factor of weights
	ErrorBufferInterface* m_errorhnd;				///< buffer for error reporting
};


class SummarizerFunctionInstanceAccumulateNear
	:public SummarizerFunctionInstanceInterface
{
public:
	explicit SummarizerFunctionInstanceAccumulateNear( ErrorBufferInterface* errorhnd_)
		:m_parameter(),m_errorhnd(errorhnd_){}

	virtual ~SummarizerFunctionInstanceAccumulateNear(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const NumericVariant& value);

	virtual std::vector<std::string> getVariables() const
	{
		return std::vector<std::string>();
	}

	virtual SummarizerFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage,
			const GlobalStatistics&) const;

	virtual bool doPopulate() const
	{
		return true;
	}
	virtual const char* name() const;
	virtual StructView view() const;

private:
	SummarizerFunctionParameterAccumulateNear m_parameter;	///< summarizer function parameters
	ErrorBufferInterface* m_errorhnd;			///< buffer for error messages
};


class SummarizerFunctionAccumulateNear
	:public SummarizerFunctionInterface
{
public:
	explicit SummarizerFunctionAccumulateNear( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	virtual ~SummarizerFunctionAccumulateNear(){}

	virtual SummarizerFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface* processor) const;

	virtual const char* name() const;
	virtual StructView view() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif


