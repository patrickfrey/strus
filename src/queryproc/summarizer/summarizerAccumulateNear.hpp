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

#define DIST_WEIGHT_BASE 2.0

struct AccumulateNearData
{
	std::string type;		//< forward index type
	std::string resultname;		//< name of result summary elements (default is same as type)
	unsigned int cardinality;	//< cardinality (minimum number of features in a window weighted)
	float cardinality_frac;		//< cardinality defined as fraction (percentage) of the number of features
	Index range;			//< maximum distance (ordinal position)
	unsigned int nofranks;		//< maximum number of ranks per document
	float cofactor;			//< weight multiplication factor of results for feature references
	double norm;			//< normalization factor for end result weights
	double cprop;			//< proportional const part of weight increment

	AccumulateNearData()
		:type(),resultname(),cardinality(0),cardinality_frac(0.0),range(0),nofranks(20),cofactor(1.0),norm(1.0),cprop(0.3){}
	AccumulateNearData( const AccumulateNearData& o)
		:type(o.type),resultname(o.resultname),cardinality(o.cardinality),cardinality_frac(o.cardinality_frac),range(o.range),nofranks(o.nofranks),cofactor(o.cofactor),norm(o.norm),cprop(o.cprop){}
};

class SummarizerFunctionContextAccumulateNear
	:public SummarizerFunctionContextInterface
{
public:
	/// \param[in] storage_ storage to use
	/// \param[in] processor_ query processor to use
	/// \param[in] data_ parameter data for evaluation
	SummarizerFunctionContextAccumulateNear(
			const StorageClientInterface* storage_,
			const QueryProcessorInterface* processor_,
			const Reference<AccumulateNearData>& data_,
			double nofCollectionDocuments_,
			ErrorBufferInterface* errorhnd_);

	virtual ~SummarizerFunctionContextAccumulateNear(){}

	virtual void addSummarizationFeature(
			const std::string& name,
			PostingIteratorInterface* itr,
			const std::vector<SummarizationVariable>& variables,
			double weight,
			const TermStatistics&);

	virtual void setVariableValue( const std::string& name, double value);

	virtual std::vector<SummaryElement> getSummary( const Index& docno);

	virtual std::string debugCall( const Index& docno);

private:
	void initializeContext();

	struct CandidateEntity
	{
		CandidateEntity()
			:forwardpos(0),structpos(0),windowendpos(0),window(0),windowsize(0){}

		Index forwardpos;		///< position of entityin forward index
		Index structpos;		///< end of structure to search for forward index matches
		Index windowendpos;		///< start position for skip to next window
		const std::size_t* window;	///< array of window element indices (posting indices)
		std::size_t windowsize;		///< size of PositionWindow in element count
	};
	bool getCandidateEntity( CandidateEntity& entityloc, const PositionWindow& poswin,
					PostingIteratorInterface** valid_itrar,
					PostingIteratorInterface** valid_structar);
	double candidateWeight( const CandidateEntity& entityloc, PostingIteratorInterface** valid_itrar) const;

	typedef std::map<std::string,double> EntityMap;
	void initEntityMap( EntityMap& emap, const Index& docno);
	std::vector<SummaryElement> getSummariesFromEntityMap( EntityMap& emap) const;

private:
	const StorageClientInterface* m_storage;			///< storage to access
	const QueryProcessorInterface* m_processor;			///< query processor interface for object creation
	Reference<ForwardIteratorInterface>m_forwardindex;		///< forward index iterators for extracting features
	Reference<AccumulateNearData> m_data;				///< parameters
	enum {MaxNofArguments=256};
	double m_nofCollectionDocuments;				///< number of documents in the collection
	ProximityWeightAccumulator::WeightArray m_idfar;		///< array of idfs
	PostingIteratorInterface* m_itrar[ MaxNofArguments];		///< array if weighted features
	PostingIteratorInterface* m_structar[ MaxNofArguments];		///< array of end of structure elements
	double m_normfactorar[ MaxNofArguments];			///< normalization factor punishing missing features
	std::size_t m_itrarsize;					///< number of weighted features
	std::size_t m_structarsize;					///< number of end of structure elements
	unsigned int m_cardinality;					///< calculated cardinality
	unsigned int m_minwinsize;					///< minimum number of elements in a window
	ProximityWeightAccumulator::WeightArray m_weightincr;		///< array of proportional weight increments 
	bool m_initialized;						///< true, if the structures have already been initialized
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


class SummarizerFunctionInstanceAccumulateNear
	:public SummarizerFunctionInstanceInterface
{
public:
	SummarizerFunctionInstanceAccumulateNear( const QueryProcessorInterface* processor_, ErrorBufferInterface* errorhnd_)
		:m_data(new AccumulateNearData()),m_processor(processor_),m_errorhnd(errorhnd_){}

	virtual ~SummarizerFunctionInstanceAccumulateNear(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const NumericVariant& value);

	virtual void defineResultName(
			const std::string& resultname,
			const std::string& itemname);

	virtual std::vector<std::string> getVariables() const
	{
		return std::vector<std::string>();
	}

	virtual SummarizerFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage,
			const GlobalStatistics&) const;

	virtual const char* name() const {return "accunear";}
	virtual StructView view() const;

private:
	Reference<AccumulateNearData> m_data;
	const QueryProcessorInterface* m_processor;
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
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

	virtual const char* name() const {return "accunear";}
	virtual StructView view() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif


