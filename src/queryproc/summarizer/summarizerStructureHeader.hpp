/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_SUMMARIZER_STRUCTURE_HEADER_HPP_INCLUDED
#define _STRUS_SUMMARIZER_STRUCTURE_HEADER_HPP_INCLUDED
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/summarizerFunctionInstanceInterface.hpp"
#include "strus/summarizerFunctionContextInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/summarizationVariable.hpp"
#include "strus/reference.hpp"
#include "proximityWeightingContext.hpp"
#include "forwardIndexTextCollector.hpp"
#include "strus/structIteratorInterface.hpp"
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

struct SummarizerFunctionParameterStructureHeader
{
	typedef ProximityWeightingContext::Config ProximityWeightingConfig;
	struct CollectorConfig
	{
		std::string name;
		std::vector<std::string> collectTypes;
		std::string tagType;
		char tagSeparator;

		CollectorConfig( const std::string& name_, const std::vector<std::string>& collectTypes_, const std::string& tagType_, char tagSeparator_)
			:name(name_),collectTypes(collectTypes_),tagType(tagType_),tagSeparator(tagSeparator_){}
		CollectorConfig( const CollectorConfig& o)
			:name(o.name),collectTypes(o.collectTypes),tagType(o.tagType),tagSeparator(o.tagSeparator){}

		StructView view() const;
	};

	ProximityWeightingConfig proximityConfig;		///< configuration for proximity weighting
	int maxNofResults;					///< maximum number of weighted features returned
	int distance_collect;					///< distance to matches to collect
	double maxdf;						///< the maximum df of features not to be considered stopwords as fraction of the total collection size
	std::vector<CollectorConfig> collectorConfigs;		///< Grouped features from forward index to collect

	SummarizerFunctionParameterStructureHeader()
		:proximityConfig()
		,maxNofResults(20)
		,distance_collect(8)
		,maxdf(0.5)
		,collectorConfigs()
	{}
	SummarizerFunctionParameterStructureHeader( const SummarizerFunctionParameterStructureHeader& o)
		:proximityConfig(o.proximityConfig)
		,maxNofResults(o.maxNofResults)
		,distance_collect(o.distance_collect)
		,maxdf(o.maxdf)
		,collectorConfigs(o.collectorConfigs)
	{}

	void addConfig( const std::string& configstr, ErrorBufferInterface* errorhnd);
};

class SummarizerFunctionContextStructureHeader
	:public SummarizerFunctionContextInterface
{
public:
	SummarizerFunctionContextStructureHeader(
			const StorageClientInterface* storage_,
			const SummarizerFunctionParameterStructureHeader& parameter_,
			double nofCollectionDocuments_,
			ErrorBufferInterface* errorhnd_);

	virtual ~SummarizerFunctionContextStructureHeader(){}

	virtual void addSummarizationFeature(
			const std::string& name,
			PostingIteratorInterface* itr,
			const std::vector<SummarizationVariable>& variables,
			double weight,
			const TermStatistics&);

	virtual void setVariableValue( const std::string& name, double value);

	virtual std::vector<SummaryElement> getSummary( const strus::WeightedDocument& doc);

	virtual std::string debugCall( const strus::WeightedDocument& doc);

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
	SummarizerFunctionParameterStructureHeader m_parameter;		///< parameter
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


class SummarizerFunctionInstanceStructureHeader
	:public SummarizerFunctionInstanceInterface
{
public:
	explicit SummarizerFunctionInstanceStructureHeader( ErrorBufferInterface* errorhnd_)
		:m_parameter(),m_errorhnd(errorhnd_){}

	virtual ~SummarizerFunctionInstanceStructureHeader(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const NumericVariant& value);

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
	SummarizerFunctionParameterStructureHeader m_parameter;	///< summarizer function parameters
	ErrorBufferInterface* m_errorhnd;			///< buffer for error messages
};


class SummarizerFunctionStructureHeader
	:public SummarizerFunctionInterface
{
public:
	explicit SummarizerFunctionStructureHeader( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	virtual ~SummarizerFunctionStructureHeader(){}

	virtual SummarizerFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface* processor) const;

	virtual const char* name() const {return "accunear";}
	virtual StructView view() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif


