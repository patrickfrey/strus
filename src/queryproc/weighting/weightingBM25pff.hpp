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
#include "strus/constants.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "private/internationalization.hpp"
#include "proximityWeightAccumulator.hpp"
#include "sentenceIterator.hpp"
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
	int maxNofResults;			///< maximum number of weighted fields returned
	double k1;				///< k1 value of BM25
	double b;				///< b value of BM25
	double avgDocLength;			///< average document length in the collection
	unsigned int paragraphsize;		///< search area for end of paragraph
	unsigned int sentencesize;		///< search area for end of sentence
	unsigned int windowsize;		///< maximum position range of a window considered for weighting
	unsigned int cardinality;		///< minumum number of features in a window considered for weighting
	float cardinality_frac;			///< cardinality defined as fraction (percentage) of the number of features
	double ffbase;				///< relative constant base factor of pure ff [0..1]
	double maxdf;				///< the maximum df of features considered for proximity weighing as fraction of the total collection size
	double titleinc;			///< ff increment for title features
	double weight_same_sentence;		///< factor for weighting same sentences
	double weight_invdist;			///< factor for weighting proximity
	double weight_invpos_start;		///< factor for weighting distance to document start
	double weight_invpos_para;		///< factor for weighting distance to last paragraph start
	double weight_invpos_struct;		///< factor for weighting distance to last sentence start
	double prop_weight_const;		///< constant factor for proportional feature weight [0.0 .. 1.0]

	WeightingFunctionParameterBM25pff()
		:maxNofResults(1)
		,k1(1.5),b(0.75),avgDocLength(500)
		,paragraphsize(300)
		,sentencesize(100)
		,windowsize(100)
		,cardinality(0),cardinality_frac(0.0)
		,ffbase(0.4)
		,maxdf(0.5)
		,titleinc(0.0)
		,weight_same_sentence(1.0)
		,weight_invdist(1.0)
		,weight_invpos_start(1.5)
		,weight_invpos_para(0.3)
		,weight_invpos_struct(0.5)
		,prop_weight_const(0.3)
	{}

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

	virtual void setVariableValue( const std::string& name_, double value);

	virtual const std::vector<WeightedField>& call( const Index& docno);

	virtual std::string debugCall( const Index& docno);

public:
	enum {MaxNofArguments=64};				///< maximum number of arguments fix because of extensive use of fixed size arrays

private:
	struct WeightingData
	{
		WeightingData( std::size_t itrarsize_, std::size_t structarsize_, std::size_t paraarsize_, strus::Index structwindowsize_, strus::Index parawindowsize_)
			:doclen(0),titlestart(1),titleend(1),ffincrar( itrarsize_,0.0)
		{
			valid_paraar = &valid_structar[ structarsize_];
			paraiter.init( parawindowsize_, valid_paraar, paraarsize_);
			structiter.init( structwindowsize_, valid_structar, structarsize_);
		}

		PostingIteratorInterface* valid_itrar[ MaxNofArguments];	//< valid array if weighted features
		PostingIteratorInterface* valid_structar[ MaxNofArguments];	//< valid array of end of structure elements
		PostingIteratorInterface** valid_paraar;			//< valid array of end of paragraph elements
		double doclen;							//< length of the document
		Index titlestart;						//< start position of the title
		Index titleend;							//< end position of the title (first item after the title)
		SentenceIterator paraiter;					//< iterator on paragraph frames
		SentenceIterator structiter;					//< iterator on sentence frames
		ProximityWeightAccumulator::WeightArray ffincrar;		//< proximity and structure increment of ff
	};

private:
	void initializeContext();
	void initWeightingData( WeightingData& data, strus::Index docno);

	void calcWindowWeight(
			WeightingData& wdata, const PositionWindow& poswin,
			const strus::IndexRange& structframe,
			const strus::IndexRange& paraframe,
			ProximityWeightAccumulator::WeightArray& result);

	double featureWeight( const WeightingData& wdata, strus::Index docno, double idf, double weight_ff) const;

	typedef ProximityWeightAccumulator::WeightArray WeightArray;
	void calcProximityFfIncrements( WeightingData& wdata, WeightArray& result);
	void logCalcProximityFfIncrements( std::ostream& out, WeightingData& wdata, ProximityWeightAccumulator::WeightArray& result);
	void calcTitleFfIncrements( WeightingData& wdata, WeightArray& result);
	void calcTitleWeights( WeightingData& wdata, strus::Index docno, WeightArray& weightar);

private:
	WeightingFunctionParameterBM25pff m_parameter;		///< weighting function parameters
	double m_nofCollectionDocuments;			///< number of documents in the collection
	unsigned int m_cardinality;				///< calculated cardinality
	ProximityWeightAccumulator::WeightArray m_idfar;	///< array of idfs
	PostingIteratorInterface* m_itrar[ MaxNofArguments];	///< array if weighted features
	PostingIteratorInterface* m_structar[ MaxNofArguments];	///< array of end of structure elements
	std::size_t m_itrarsize;				///< number of weighted features
	std::size_t m_structarsize;				///< number of end of structure elements
	std::size_t m_paraarsize;				///< number of paragraph elements (now summary accross paragraph borders)
	std::size_t m_nof_maxdf_features;			///< number of features with a df bigger than maximum
	bool m_relevantfeat[ MaxNofArguments];			///< marker for features with a df smaller than maxdf
	ProximityWeightAccumulator::WeightArray m_weightincr;	///< array of proportional weight increments 
	bool m_initialized;					///< true, if the structures have already been initialized
	strus::Reference<MetaDataReaderInterface> m_metadata;	///< meta data reader
	int m_metadata_doclen;					///< meta data doclen handle
	PostingIteratorInterface* m_titleitr;			///< iterator to identify the title field for weight increment
	std::vector<WeightedField> m_lastResult;		///< buffer for the last result calculated
	ErrorBufferInterface* m_errorhnd;			///< buffer for error messages
};


/// \class WeightingFunctionInstanceBM25pff
/// \brief Weighting function instance based on the BM25pff formula
class WeightingFunctionInstanceBM25pff
	:public WeightingFunctionInstanceInterface
{
public:
	explicit WeightingFunctionInstanceBM25pff( ErrorBufferInterface* errorhnd_)
		:m_parameter(),m_metadata_doclen(strus::Constants::standard_metadata_document_length()),m_errorhnd(errorhnd_){}

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

	virtual const char* name() const {return "bm25pff";}
	virtual StructView view() const;

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

	virtual const char* name() const {return "bm25pff";}
	virtual StructView view() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif

