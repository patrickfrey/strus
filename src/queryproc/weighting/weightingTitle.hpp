/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_WEIGHTING_TITLE_HPP_INCLUDED
#define _STRUS_WEIGHTING_TITLE_HPP_INCLUDED
#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/weightingFunctionContextInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/storage/index.hpp"
#include "strus/reference.hpp"
#include "strus/constants.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/structureIteratorInterface.hpp"
#include "private/internationalization.hpp"
#include <vector>
#include <cstring>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <utility>

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;

/// \class WeightingFunctionContextTitle
/// \brief Context of weighting function calculating a weight for features appearing in the title and in headings
class WeightingFunctionContextTitle
	:public WeightingFunctionContextInterface
{
public:
	WeightingFunctionContextTitle(
			const StorageClientInterface* storage,
			double hierarchyWeightFactor_,
			int maxNofResults_,
			double maxdf_,
			double nofCollectionDocuments_,
			ErrorBufferInterface* errorhnd_);
	
	~WeightingFunctionContextTitle(){}

	virtual void addWeightingFeature(
			const std::string& name_,
			PostingIteratorInterface* itr_,
			double weight_,
			const TermStatistics& stats_);

	virtual void setVariableValue( const std::string& name_, double value);

	virtual const std::vector<WeightedField>& call( const Index& docno);

private:
	int tryQuerySequenceMatchToField( int postingIdx, const strus::IndexRange& field);

public:
	enum {
		MaxNofArguments=64,	///< maximum number of arguments fix because of extensive use of fixed size arrays
		MaxTitleSize=64		///< maximum title size considered as worth for weighting
	};
	
private:
	const StorageClientInterface* m_storage;			///< storage client interface
	strus::Reference<StructureIteratorInterface> m_structitr;	///< structure iterator to recognize titles
	PostingIteratorInterface* m_postingar[ MaxNofArguments];	///< array of weighted feature postings
	int m_postingarsize;						///< number of postings defined
	bool m_isStopWordAr[ MaxNofArguments];				///< parallel array to m_postingar that tells if the posting is a stopword
	double m_hierarchyWeightFactor;					///< value between 0 and 1, factor a sub title feature is multiplied with, describes the weight loss of a sub title feature against a title feature
	double m_hierarchyWeight[ strus::Constants::MaxStructLevels];	///< weight for each structure level
	double m_maxdf;							///< the maximum df of features not to be considered stopwords as fraction of the total collection size
	int m_maxNofResults;						///< maximum number of results
	double m_nofCollectionDocuments;				///< number of documents in the collection
	std::vector<WeightedField> m_lastResult;			///< buffer for the last result calculated
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

/// \class WeightingFunctionInstanceTitle
/// \brief Instance of weighting function calculating a weight for features appearing in the title and in headings
class WeightingFunctionInstanceTitle
	:public WeightingFunctionInstanceInterface
{
public:
	explicit WeightingFunctionInstanceTitle( ErrorBufferInterface* errorhnd_)
		:m_hierarchyWeightFactor(0.7),m_maxNofResults(5),m_maxdf(1.0),m_errorhnd(errorhnd_){}

	virtual ~WeightingFunctionInstanceTitle(){}

	virtual void addStringParameter( const std::string& name_, const std::string& value);
	virtual void addNumericParameter( const std::string& name_, const NumericVariant& value);

	virtual std::vector<std::string> getVariables() const
	{
		return std::vector<std::string>();
	}

	virtual WeightingFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage_,
			const GlobalStatistics& stats) const;

	virtual const char* name() const {return "title";}
	virtual StructView view() const;

private:
	double m_hierarchyWeightFactor;			///< value between 0 and 1, factor a sub title feature is multiplied with, describes the weight loss of a sub title feature against a title feature
	int m_maxNofResults;				///< maximum number of results
	double m_maxdf;					///< the maximum df of features not to be considered stopwords as fraction of the total collection size
	ErrorBufferInterface* m_errorhnd;		///< buffer for error messages
};


/// \class WeightingFunctionTitle
/// \brief Weighting function calculating a weight for features appearing in the title and in headings
class WeightingFunctionTitle
	:public WeightingFunctionInterface
{
public:
	explicit WeightingFunctionTitle( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}

	virtual ~WeightingFunctionTitle(){}

	virtual WeightingFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface* processor) const;

	virtual const char* name() const {return "title";}
	virtual StructView view() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif


