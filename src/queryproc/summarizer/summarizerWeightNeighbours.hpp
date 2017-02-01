/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_SUMMARIZER_WEIGHT_NEIGHBOURS_HPP_INCLUDED
#define _STRUS_SUMMARIZER_WEIGHT_NEIGHBOURS_HPP_INCLUDED
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/summarizerFunctionInstanceInterface.hpp"
#include "strus/summarizerFunctionContextInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/summarizationVariable.hpp"
#include "strus/reference.hpp"
#include "private/internationalization.hpp"
#include "proximityWeightAccumulator.hpp"
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

struct WeightNeighboursData
{
	std::string type;		//< forward index type
	std::string resultname;		//< name of result summary elements (default is same as type)
	unsigned int cardinality;	//< cardinality (minimum number of features in a window weighted)
	Index range;			//< maximum distance (ordinal position)
	unsigned int nofranks;		//< maximum number of ranks per document
	float cofactor;			//< weight multiplication factor of results for feature references

	WeightNeighboursData()
		:type(),resultname(),cardinality(0),range(0),nofranks(20),cofactor(1.0){}
	WeightNeighboursData( const WeightNeighboursData& o)
		:type(o.type),resultname(o.resultname),cardinality(o.cardinality),range(o.range),nofranks(o.nofranks),cofactor(o.cofactor){}
};

class SummarizerFunctionContextWeightNeighbours
	:public SummarizerFunctionContextInterface
{
public:
	/// \param[in] storage_ storage to use
	/// \param[in] processor_ query processor to use
	/// \param[in] data_ parameter data for evaluation
	SummarizerFunctionContextWeightNeighbours(
			const StorageClientInterface* storage_,
			const QueryProcessorInterface* processor_,
			const Reference<WeightNeighboursData>& data_,
			double nofCollectionDocuments_,
			ErrorBufferInterface* errorhnd_);

	virtual ~SummarizerFunctionContextWeightNeighbours(){}

	virtual void addSummarizationFeature(
			const std::string& name,
			PostingIteratorInterface* itr,
			const std::vector<SummarizationVariable>& variables,
			double weight,
			const TermStatistics&);

	virtual std::vector<SummaryElement> getSummary( const Index& docno);

private:
	const StorageClientInterface* m_storage;			///< storage to access
	const QueryProcessorInterface* m_processor;			///< query processor interface for object creation
	Reference<ForwardIteratorInterface>m_forwardindex;		///< forward index iterators for extracting features
	Reference<WeightNeighboursData> m_data;				///< parameters
	enum {MaxNofArguments=256};
	double m_nofCollectionDocuments;				///< number of documents in the collection
	ProximityWeightAccumulator::WeightArray m_idfar;		///< array of idfs
	PostingIteratorInterface* m_itrar[ MaxNofArguments];		///< array if weighted features
	PostingIteratorInterface* m_structar[ MaxNofArguments];		///< array of end of structure elements
	double m_normfactorar[ MaxNofArguments];			///< normalization factor punishing missing features
	std::size_t m_itrarsize;					///< number of weighted features
	std::size_t m_structarsize;					///< number of end of structure elements
	ProximityWeightAccumulator::WeightArray m_weightincr;		///< array of proportional weight increments 
	bool m_initialized;						///< true, if the structures have already been initialized
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


class SummarizerFunctionInstanceWeightNeighbours
	:public SummarizerFunctionInstanceInterface
{
public:
	SummarizerFunctionInstanceWeightNeighbours( const QueryProcessorInterface* processor_, ErrorBufferInterface* errorhnd_)
		:m_data(new WeightNeighboursData()),m_processor(processor_),m_errorhnd(errorhnd_){}

	virtual ~SummarizerFunctionInstanceWeightNeighbours(){}

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
	Reference<WeightNeighboursData> m_data;
	const QueryProcessorInterface* m_processor;
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


class SummarizerFunctionWeightNeighbours
	:public SummarizerFunctionInterface
{
public:
	explicit SummarizerFunctionWeightNeighbours( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	virtual ~SummarizerFunctionWeightNeighbours(){}

	virtual SummarizerFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface* processor) const;

	virtual FunctionDescription getDescription() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif


