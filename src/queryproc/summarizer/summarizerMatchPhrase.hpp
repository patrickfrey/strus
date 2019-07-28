/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_SUMMARIZER_MATCH_PHRASE_HPP_INCLUDED
#define _STRUS_SUMMARIZER_MATCH_PHRASE_HPP_INCLUDED
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/summarizerFunctionInstanceInterface.hpp"
#include "strus/summarizerFunctionContextInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/reference.hpp"
#include "private/internationalization.hpp"
#include "proximityWeightAccumulator.hpp"
#include "structureIterator.hpp"
#include <vector>
#include <string>
#include <stdexcept>
#include <sstream>
#include <iostream>

namespace strus
{

/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class MetaDataReaderInterface;
/// \brief Forward declaration
class ForwardIteratorInterface;
/// \brief Forward declaration
class QueryProcessorInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

enum {
	MaxParaTitleSize=12
};

/// \brief Configured parameters of the MatchPhrase summarizer function
struct SummarizerFunctionParameterMatchPhrase
{
	SummarizerFunctionParameterMatchPhrase()
		:m_type()
		,m_paragraphsize(300)
		,m_sentencesize(100)
		,m_windowsize(100)
		,m_cardinality(0)
		,m_cardinality_frac(0.0)
		,m_maxdf(0.1)
		,m_matchmark()
		,m_floatingmark(std::pair<std::string,std::string>("... "," ..."))
		,m_name_para("para")
		,m_name_phrase("phrase")
		,m_name_docstart("docstart")
		,m_weight_same_sentence(0.6)
		,m_weight_invdist(0.6)
		,m_weight_invpos_start(2.5)
		,m_weight_invpos_para(0.3)
		,m_weight_invpos_struct(0.5)
		,m_prop_weight_const(0.3)
	{}

	std::string m_type;					///< forward index type to extract
	unsigned int m_paragraphsize;				///< search area for end of paragraph
	unsigned int m_sentencesize;				///< search area for end of sentence
	unsigned int m_windowsize;				///< maximum window size
	unsigned int m_cardinality;				///< window cardinality
	float m_cardinality_frac;				///< cardinality defined as fraction (percentage) of the number of features
	double m_maxdf;						///< the maximum df of features considered for same sentence proximity weighing as fraction of the total collection size
	std::pair<std::string,std::string> m_matchmark;		///< highlighting info
	std::pair<std::string,std::string> m_floatingmark;	///< marker for unterminated begin and end phrase
	std::string m_name_para;				///< name of the summary elements for paragraphs
	std::string m_name_phrase;				///< name of the summary elements for phrases
	std::string m_name_docstart;				///< name of the summary elements for document start (alternative summary if no match found)
	double m_weight_same_sentence;				///< factor for weighting same sentences
	double m_weight_invdist;				///< factor for weighting proximity
	double m_weight_invpos_start;				///< factor for weighting distance to document start
	double m_weight_invpos_para;				///< factor for weighting distance to last paragraph start
	double m_weight_invpos_struct;				///< factor for weighting distance to last sentence start
	double m_prop_weight_const;				///< constant factor for proportional feature weight [0.0 .. 1.0]
};

class SummarizerFunctionContextMatchPhrase
	:public SummarizerFunctionContextInterface
{
public:
	/// \param[in] storage_ storage to use
	/// \param[in] processor_ query processor to use
	/// \param[in] metadata_ meta data reader
	/// \param[in] parameter_ parameter for weighting
	/// \param[in] nofCollectionDocuments_ number of documents in the collection
	/// \param[in] errorhnd_ error buffer interface
	SummarizerFunctionContextMatchPhrase(
			const StorageClientInterface* storage_,
			const QueryProcessorInterface* processor_,
			MetaDataReaderInterface* metadata_,
			const Reference<SummarizerFunctionParameterMatchPhrase>& parameter_,
			double nofCollectionDocuments_,
			ErrorBufferInterface* errorhnd_);
	virtual ~SummarizerFunctionContextMatchPhrase();

	virtual void addSummarizationFeature(
			const std::string& name_,
			PostingIteratorInterface* itr,
			const std::vector<SummarizationVariable>&,
			double weight,
			const TermStatistics&);

	virtual void setVariableValue( const std::string& name_, double value);

	virtual std::vector<SummaryElement> getSummary( const Index& docno);

	virtual std::string debugCall( const Index& docno);

public:
	enum {MaxNofArguments=64};				///< chosen to fit in a bitfield of 64 bits

private:
	struct WeightingData
	{
		explicit WeightingData( std::size_t structarsize_, std::size_t paraarsize_, const Index& structwindowsize_, const Index& parawindowsize_)
			:titlestart(1),titleend(1)
		{
			valid_paraar = &valid_structar[ structarsize_];
			paraiter.init( parawindowsize_, valid_paraar, paraarsize_);
			structiter.init( structwindowsize_, valid_structar, structarsize_);
		}

		PostingIteratorInterface* valid_itrar[ MaxNofArguments];	//< valid array if weighted features
		PostingIteratorInterface* valid_structar[ MaxNofArguments];	//< valid array of end of structure elements
		PostingIteratorInterface** valid_paraar;			//< valid array of end of paragraph elements
		Index titlestart;						//< start position of the title
		Index titleend;							//< end position of the title (first item after the title)
		StructureIterator paraiter;					//< iterator on paragraph frames
		StructureIterator structiter;					//< iterator on sentence frames
	};

private:
	void initializeContext();
	void initWeightingData( WeightingData& wdata, const Index& docno);

private:
	struct Match
	{
		double weight;
		Index pos;
		Index span;
		bool is_docstart;
	
		Match()
			:weight(0.0),pos(0),span(0),is_docstart(false){}
		Match( double weight_, Index pos_, Index span_, bool is_docstart_)
			:weight(weight_),pos(pos_),span(span_),is_docstart(is_docstart_){}
		Match( const Match& o)
			:weight(o.weight),pos(o.pos),span(o.span),is_docstart(o.is_docstart){}
	
		bool isDefined() const		{return span > 0;}
	};

	struct Abstract
	{
		Index start;
		Index span;
		bool defined_start;
		bool defined_end;
		bool is_docstart;

		Abstract()
			:start(0),span(0),defined_start(false),defined_end(false),is_docstart(false){}
		Abstract( const Index& start_, const Index& span_, bool defined_start_, bool defined_end_, bool is_docstart_)
			:start(start_),span(span_),defined_start(defined_start_),defined_end(defined_end_),is_docstart(is_docstart_){}
		Abstract( const Abstract& o)
			:start(o.start),span(o.span),defined_start(o.defined_start),defined_end(o.defined_end),is_docstart(o.is_docstart){}

		bool isDefined() const		{return span > 0;}
	};

	double windowWeight( WeightingData& wdata, const PositionWindow& poswin, const std::pair<Index,Index>& structframe, const std::pair<Index,Index>& paraframe);

	void fetchNoTitlePostings( WeightingData& wdata, PostingIteratorInterface** itrar, Index& cntTitleTerms, Index& cntNoTitleTerms);
	Match findBestMatch_( WeightingData& wdata, unsigned int cardinality, PostingIteratorInterface** itrar);
	Match findBestMatch( WeightingData& wdata);
	Match findBestMatchNoTitle( WeightingData& wdata);
	Match findAbstractMatch( WeightingData& wdata);

	Match logFindBestMatch_( std::ostream& out, WeightingData& wdata, unsigned int cardinality, PostingIteratorInterface** itrar);
	Match logFindBestMatchNoTitle( std::ostream& out, WeightingData& wdata);
	Match logFindAbstractMatch( std::ostream& out, WeightingData& wdata);

	Abstract getPhraseAbstract( const Match& candidate, WeightingData& wdata);
	Abstract getParaTitleAbstract( Match& phrase_match, WeightingData& wdata);

	std::string getParaTitleString( const Abstract& para_abstract);
	std::string getPhraseString( const Abstract& phrase_abstract, WeightingData& wdata);
	std::string getPhraseString( const Index& firstpos, const Index& lastpos);

	std::vector<SummaryElement>
		getSummariesFromAbstracts(
			const Abstract& para_abstract,
			const Abstract& phrase_abstract,
			WeightingData& wdata);

private:
	const StorageClientInterface* m_storage;		///< storage access
	const QueryProcessorInterface* m_processor;		///< query processor interface
	MetaDataReaderInterface* m_metadata;			///< access metadata arguments
	Reference<ForwardIteratorInterface> m_forwardindex;	///< forward index iterator
	Reference<SummarizerFunctionParameterMatchPhrase> m_parameter;
	double m_nofCollectionDocuments;			///< number of documents in the collection
	ProximityWeightAccumulator::WeightArray m_idfar;	///< array of idfs
	PostingIteratorInterface* m_itrar[ MaxNofArguments];	///< array if weighted features
	PostingIteratorInterface* m_structar[ MaxNofArguments];	///< array of end of structure elements
	std::size_t m_itrarsize;				///< number of weighted features
	std::size_t m_structarsize;				///< number of end of structure elements
	std::size_t m_paraarsize;				///< number of paragraph elements (now summary accross paragraph borders)
	std::size_t m_nof_maxdf_features;			///< number of features with a df bigger than maximum
	unsigned int m_cardinality;				///< calculated cardinality
	ProximityWeightAccumulator::WeightArray m_weightincr;	///< array of proportional weight increments 
	bool m_initialized;					///< true, if the structures have already been initialized
	PostingIteratorInterface* m_titleitr;			///< iterator to identify the title field for weight increment
	ErrorBufferInterface* m_errorhnd;			///< buffer for error messages
};


/// \class SummarizerFunctionInstanceMatchPhrase
/// \brief Summarizer instance for retrieving meta data
class SummarizerFunctionInstanceMatchPhrase
	:public SummarizerFunctionInstanceInterface
{
public:
	SummarizerFunctionInstanceMatchPhrase( const QueryProcessorInterface* processor_, ErrorBufferInterface* errorhnd_)
		:m_parameter( new SummarizerFunctionParameterMatchPhrase())
		,m_processor(processor_)
		,m_errorhnd(errorhnd_){}

	virtual ~SummarizerFunctionInstanceMatchPhrase(){}

	virtual void addStringParameter( const std::string& name_, const std::string& value);
	virtual void addNumericParameter( const std::string& name_, const NumericVariant& value);

	virtual void defineResultName(
			const std::string& resultname,
			const std::string& itemname);

	virtual std::vector<std::string> getVariables() const
	{
		return std::vector<std::string>();
	}

	virtual SummarizerFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage,
			MetaDataReaderInterface*,
			const GlobalStatistics&) const;

	virtual const char* name() const {return "matchphrase";}
	virtual StructView view() const;

private:
	Reference<SummarizerFunctionParameterMatchPhrase> m_parameter;
	const QueryProcessorInterface* m_processor;		///< query processor interface
	ErrorBufferInterface* m_errorhnd;			///< buffer for error messages
};


class SummarizerFunctionMatchPhrase
	:public SummarizerFunctionInterface
{
public:
	explicit SummarizerFunctionMatchPhrase( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}

	virtual ~SummarizerFunctionMatchPhrase(){}

	virtual SummarizerFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface* processor) const;

	virtual const char* name() const {return "matchphrase";}
	virtual StructView view() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif


