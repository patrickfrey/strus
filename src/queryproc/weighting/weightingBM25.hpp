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
#include "strus/constants.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "private/internationalization.hpp"
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Configured parameters of the BM25 weighting function
struct WeightingFunctionParameterBM25
{
	double k1;				///< k1 value of BM25
	double b;				///< b value of BM25
	double avgDocLength;			///< average document length in the collection

	WeightingFunctionParameterBM25()
		:k1(1.5),b(0.75),avgDocLength(500){}

	WeightingFunctionParameterBM25( const WeightingFunctionParameterBM25& o)
	{
		std::memcpy( this, &o, sizeof(*this));
	}
};

/// \class WeightingFunctionContextBM25
/// \brief Weighting function based on the BM25 formula
class WeightingFunctionContextBM25
	:public WeightingFunctionContextInterface
{
public:
	WeightingFunctionContextBM25(
		const StorageClientInterface* storage,
		MetaDataReaderInterface* metadata_,
		const WeightingFunctionParameterBM25& parameter_,
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
			double weight_,
			const TermStatistics& stats_);

	virtual void setVariableValue( const std::string& name, double value);

	virtual double call( const Index& docno);

	virtual std::string debugCall( const Index& docno);

private:
	double featureWeight( const Feature& feat, const Index& docno) const;

private:
	WeightingFunctionParameterBM25 m_parameter;
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
		:m_parameter(),m_metadata_doclen(strus::Constants::standard_metadata_document_length()),m_errorhnd(errorhnd_){}

	virtual ~WeightingFunctionInstanceBM25(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const NumericVariant& value);

	virtual std::vector<std::string> getVariables() const
	{
		return std::vector<std::string>();
	}

	virtual WeightingFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage_,
			MetaDataReaderInterface* metadata,
			const GlobalStatistics& stats) const;

	virtual StructView view() const;

private:
	WeightingFunctionParameterBM25 m_parameter;	///< configured weighting function parameters
	std::string m_metadata_doclen;			///< document metadata element of the document length
	ErrorBufferInterface* m_errorhnd;		///< buffer for error messages
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

