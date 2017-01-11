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
#include "strus/storageClientInterface.hpp"
#include "strus/index.hpp"
#include "strus/reference.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "private/internationalization.hpp"
#include "private/utils.hpp"
#include "proximityWeightAccumulator.hpp"
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
	double k1;				///< k1 value of BM25
	double b;				///< b value of BM25
	double avgDocLength;			///< average document length in the collection
	unsigned int windowsize;		///< maximum position range of a window considered for weighting
	unsigned int cardinality;		///< minumum number of features in a window considered for weighting
	double ffbase;				///< relative constant base factor of pure ff [0..1]
	unsigned int fftie;			///< the maximum pure ff value that is considered for weighting (used for normalization of pure ff part)
	double proxffbias;			///< bias for proximity ff increments always counted (the others are counted only till m_proxfftie)
	unsigned int proxfftie;			///< the maximum proximity based ff value that is considered for weighting except for increments exceeding m_proxffbias
	double maxdf;				///< the maximum df of features considered for proximity weighing as fraction of the total collection size
	double titleinc;			///< ff increment for title features
	unsigned int tidocnorm;			///< the document size used for calibrating the title match weight normalization between 0 and 1 (0->0 titleinc_docsizenorm and bigger->1). This weight is a measure of how much information a document should contain so that query terms in the title can be weighted.

	WeightingFunctionParameterBM25pff()
		:k1(1.5),b(0.75),avgDocLength(500)
		,windowsize(100),cardinality(0)
		,ffbase(0.4),fftie(0)
		,proxffbias(0.0)
		,proxfftie(0)
		,maxdf(0.5)
		,titleinc(0.0),tidocnorm(0){}

	WeightingFunctionParameterBM25pff( const WeightingFunctionParameterBM25pff& o)
	{
		std::memcpy( this, &o, sizeof(*this));
	}
};

/// \class WeightingFunctionContextBM25pff
/// \brief Weighting function based on the BM25pff (BM25 with proximity feature frequency) formula
class WeightingFunctionContextBM25pff
	:public WeightingFunctionContextInterface
{
public:
	WeightingFunctionContextBM25pff(
		const StorageClientInterface* storage,
		MetaDataReaderInterface* metadata_,
		const WeightingFunctionParameterBM25pff& parameter_,
		double nofCollectionDocuments_,
		const std::string& metadata_doclen_,
		ErrorBufferInterface* errorhnd_);
	~WeightingFunctionContextBM25pff(){}

	virtual void addWeightingFeature(
			const std::string& name_,
			PostingIteratorInterface* itr_,
			double weight_,
			const TermStatistics& stats_);

	virtual double call( const Index& docno);

public:
	enum {MaxNofArguments=64};				///< maximum number of arguments fix because of extensive use of fixed size arrays

private:
	WeightingFunctionParameterBM25pff m_parameter;		///< weighting function parameters
	double m_nofCollectionDocuments;			///< number of documents in the collection
	ProximityWeightAccumulator::WeightArray m_idfar;	///< array of idfs
	PostingIteratorInterface* m_itrar[ MaxNofArguments];	///< array if weighted features
	PostingIteratorInterface* m_structar[ MaxNofArguments];	///< array of end of structure elements
	PostingIteratorInterface* m_paraar[ MaxNofArguments];	///< array of end of paragraph elements
	std::size_t m_itrarsize;				///< number of weighted features
	std::size_t m_structarsize;				///< number of end of structure elements
	std::size_t m_paraarsize;				///< number of paragraph elements (now summary accross paragraph borders)
	std::size_t m_nof_maxdf_features;			///< number of features with a df bigger than maximum
	Index m_maxdist_featar[ MaxNofArguments];		///< array of distances indicating what proximity distance is considered at maximum for same sentence weight
	double m_normfactorar[ MaxNofArguments];		///< normalization factor taking missing features in a window into account
	ProximityWeightAccumulator::WeightArray m_weightincr;	///< array of proportional weight increments 
	bool m_initialized;					///< true, if the structures have already been initialized
	MetaDataReaderInterface* m_metadata;			///< meta data reader
	int m_metadata_doclen;					///< meta data doclen handle
	PostingIteratorInterface* m_titleitr;			///< iterator to identify the title field for weight increment
	ErrorBufferInterface* m_errorhnd;			///< buffer for error messages
};


/// \class WeightingFunctionInstanceBM25pff
/// \brief Weighting function instance based on the BM25pff formula
class WeightingFunctionInstanceBM25pff
	:public WeightingFunctionInstanceInterface
{
public:
	explicit WeightingFunctionInstanceBM25pff( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}

	virtual ~WeightingFunctionInstanceBM25pff(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const NumericVariant& value);

	virtual WeightingFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage_,
			MetaDataReaderInterface* metadata,
			const GlobalStatistics& stats) const;

	virtual std::string tostring() const;

private:
	WeightingFunctionParameterBM25pff m_parameter;	///< weighting function parameters
	std::string m_metadata_doclen;			///< attribute defining the document length
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

	virtual FunctionDescription getDescription() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif

