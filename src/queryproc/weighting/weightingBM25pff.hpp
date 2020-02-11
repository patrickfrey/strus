/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_WEIGHTING_BM25PFF_HPP_INCLUDED
#define _STRUS_WEIGHTING_BM25PFF_HPP_INCLUDED
#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/weightingFunctionContextInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/structureIteratorInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/index.hpp"
#include "strus/reference.hpp"
#include "strus/constants.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "private/internationalization.hpp"
#include "proximityWeightingContext.hpp"
#include <vector>
#include <cstring>
#include <sstream>
#include <iostream>
#include <iomanip>

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Configured parameters of the BM25pff weighting function
struct WeightingFunctionParameterBM25pff
{
	typedef ProximityWeightingContext::Config ProximityWeightingConfig;
	ProximityWeightingConfig proximityConfig;	///< configuration for proximity weighting
	int maxNofResults;				///< maximum number of weighted fields returned
	double k1;					///< k1 value of BM25
	double b;					///< b value of BM25
	double avgDocLength;				///< average document length in the collection
	double maxdf;					///< the maximum df of features not to be considered stopwords as fraction of the total collection size

	WeightingFunctionParameterBM25pff()
		:proximityConfig()
		,maxNofResults(2)
		,k1(1.5),b(0.75),avgDocLength(500)
		,maxdf(0.5)
	{}

	WeightingFunctionParameterBM25pff( const WeightingFunctionParameterBM25pff& o)
		:proximityConfig(o.proximityConfig)
		,maxNofResults(o.maxNofResults)
		,k1(o.k1),b(o.b),avgDocLength(o.avgDocLength)
		,maxdf(o.maxdf)
	{}
	double postingsWeight( int doclen, double idf, double weight_ff) const;
};

/// \class WeightingFunctionContextBM25pff
/// \brief Weighting function based on the BM25pff (BM25 with proximity feature frequency) formula
class WeightingFunctionContextBM25pff
	:public WeightingFunctionContextInterface
{
public:
	WeightingFunctionContextBM25pff(
		const StorageClientInterface* storage,
		const WeightingFunctionParameterBM25pff& parameter_,
		double nofCollectionDocuments_,
		const std::string& metadata_doclen_,
		const std::string& structname_,
		ErrorBufferInterface* errorhnd_);
	~WeightingFunctionContextBM25pff(){}

	virtual void addWeightingFeature(
			const std::string& name_,
			PostingIteratorInterface* itr_,
			double weight_,
			const TermStatistics& stats_);

	virtual void setVariableValue( const std::string& name_, double value);

	virtual const std::vector<WeightedField>& call( const Index& docno);

public:
	enum {MaxNofArguments=ProximityWeightingContext::MaxNofArguments};	///< maximum number of arguments fix because of extensive use of fixed size arrays
	typedef ProximityWeightingContext::FeatureWeights FeatureWeights;
	typedef ProximityWeightingContext::FieldStatistics FieldStatistics;

private:
	ProximityWeightingContext m_proximityWeightingContext;		///< proximity weighting context
	WeightingFunctionParameterBM25pff m_parameter;			///< parameter
	PostingIteratorInterface* m_itrar[ MaxNofArguments];		///< posting iterators to weight
	std::size_t m_itrarsize;					///< nof posting iterators defined to weight
	FeatureWeights m_weightar;					///< array of feature weights parallel to m_itrar
	PostingIteratorInterface* m_stopword_itrar[ MaxNofArguments];	///< posting iterators to weight
	std::size_t m_stopword_itrarsize;				///< nof posting iterators defined to weight
	FeatureWeights m_stopword_weightar;				///< array of feature weights parallel to m_itrar
	PostingIteratorInterface* m_eos_itr;				///< posting iterators for end of sentence markers
	strus::Reference<StructureIteratorInterface> m_structitr;	///< structure iterator
	strus::Reference<MetaDataReaderInterface> m_metadata;		///< meta data reader
	int m_metadata_doclen;						///< meta data doclen handle
	strus::Index m_structno;					///< structure type number to use for select the weighted structures or 0 if you want to use all structures
	double m_nofCollectionDocuments;				///< number of documents in the collection
	const StorageClientInterface* m_storage;			///< storage client interface
	std::vector<WeightedField> m_lastResult;			///< buffer for the last result calculated
	ErrorBufferInterface* m_errorhnd;				///< buffer for error reporting
};


/// \class WeightingFunctionInstanceBM25pff
/// \brief Weighting function instance based on the BM25pff formula
class WeightingFunctionInstanceBM25pff
	:public WeightingFunctionInstanceInterface
{
public:
	explicit WeightingFunctionInstanceBM25pff( ErrorBufferInterface* errorhnd_)
		:m_parameter(),m_metadata_doclen(strus::Constants::standard_metadata_document_length())
		,m_structname(),m_errorhnd(errorhnd_){}

	virtual ~WeightingFunctionInstanceBM25pff(){}

	virtual void addStringParameter( const std::string& name_, const std::string& value);
	virtual void addNumericParameter( const std::string& name_, const NumericVariant& value);

	virtual std::vector<std::string> getVariables() const
	{
		return std::vector<std::string>();
	}

	virtual WeightingFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage_,
			const GlobalStatistics& stats) const;

	virtual const char* name() const;
	virtual StructView view() const;

private:
	WeightingFunctionParameterBM25pff m_parameter;	///< weighting function parameters
	std::string m_metadata_doclen;			///< attribute defining the document length
	std::string m_structname;			///< name of structure to use for detecting part of documents (fields) to weight
	ErrorBufferInterface* m_errorhnd;		///< buffer for error messages
};


/// \class WeightingFunctionBM25pff
/// \brief Weighting function based on the BM25pff formula
class WeightingFunctionBM25pff
	:public WeightingFunctionInterface
{
public:
	explicit WeightingFunctionBM25pff( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}

	virtual ~WeightingFunctionBM25pff(){}

	virtual WeightingFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface* processor) const;

	virtual const char* name() const;
	virtual StructView view() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif

