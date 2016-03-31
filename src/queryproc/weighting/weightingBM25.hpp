/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_WEIGHTING_BM25_HPP_INCLUDED
#define _STRUS_WEIGHTING_BM25_HPP_INCLUDED
#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/weightingFunctionContextInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/index.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "private/internationalization.hpp"
#include "private/utils.hpp"
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;

/// \class WeightingFunctionContextBM25
/// \brief Weighting function based on the BM25 formula
class WeightingFunctionContextBM25
	:public WeightingFunctionContextInterface
{
public:
	WeightingFunctionContextBM25(
		const StorageClientInterface* storage,
		MetaDataReaderInterface* metadata_,
		double k1_,
		double b_,
		double avgDocLength_,
		double nofCollectionDocuments_,
		const std::string& attribute_doclen_,
		ErrorBufferInterface* errorhnd_);

	struct Feature
	{
		PostingIteratorInterface* itr;
		double weight;
		double idf;

		Feature( PostingIteratorInterface* itr_, double weight_, double idf_)
			:itr(itr_),weight(weight_),idf(idf_){}
		Feature( const Feature& o)
			:itr(o.itr),weight(o.weight),idf(o.idf){}
	};

	virtual void addWeightingFeature(
			const std::string& name_,
			PostingIteratorInterface* itr_,
			float weight_,
			const TermStatistics& stats_);

	virtual double call( const Index& docno);

private:
	double m_k1;
	double m_b;
	double m_avgDocLength;
	double m_nofCollectionDocuments;
	std::vector<Feature> m_featar;
	MetaDataReaderInterface* m_metadata;
	int m_metadata_doclen;
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


/// \class WeightingFunctionInstanceBM25
/// \brief Weighting function instance based on the BM25 formula
class WeightingFunctionInstanceBM25
	:public WeightingFunctionInstanceInterface
{
public:
	explicit WeightingFunctionInstanceBM25( ErrorBufferInterface* errorhnd_)
		:m_k1(1.5),m_b(0.75),m_avgdoclen(1000),m_errorhnd(errorhnd_){}

	virtual ~WeightingFunctionInstanceBM25(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const NumericVariant& value);

	virtual WeightingFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage_,
			MetaDataReaderInterface* metadata,
			const GlobalStatistics& stats) const;

	virtual std::string tostring() const;

private:
	double m_k1;
	double m_b;
	double m_avgdoclen;
	std::string m_metadata_doclen;
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


/// \class WeightingFunctionBM25
/// \brief Weighting function based on the BM25 formula
class WeightingFunctionBM25
	:public WeightingFunctionInterface
{
public:
	explicit WeightingFunctionBM25( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}

	virtual ~WeightingFunctionBM25(){}

	virtual WeightingFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface* processor) const;

	virtual FunctionDescription getDescription() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif

