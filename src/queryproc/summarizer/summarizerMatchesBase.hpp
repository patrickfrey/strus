/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_SUMMARIZER_MATCHES_BASE_HPP_INCLUDED
#define _STRUS_SUMMARIZER_MATCHES_BASE_HPP_INCLUDED
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/summarizerFunctionInstanceInterface.hpp"
#include "strus/summarizerFunctionContextInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/storage/summarizationVariable.hpp"
#include "strus/reference.hpp"
#include "strus/base/bitset.hpp"
#include "strus/base/string_named_format.hpp"
#include "private/internationalization.hpp"
#include "private/localStructAllocator.hpp"
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <map>

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

struct MatchesBaseParameter
{
	std::string texttype;		//< forward index type describing the value
	std::string tagtype;		//< forward index type describing the tag
	std::string fmt;		//< variable to accumulate
	std::string stripCharacters;	//< characters to strip away from used forward index elements
	int maxNofElements;		//< maximum number of best elements to return

	MatchesBaseParameter()
		:texttype(),tagtype(),fmt(),stripCharacters(),maxNofElements(30){}
	MatchesBaseParameter( const MatchesBaseParameter& o)
		:texttype(o.texttype),tagtype(o.tagtype),fmt(o.fmt)
		,stripCharacters(o.stripCharacters)
		,maxNofElements(o.maxNofElements){}
};

/// \brief Base class for context implementing a summarizer listing or accumulating matches
class SummarizerFunctionContextMatchesBase
	:public SummarizerFunctionContextInterface
{
public:
	/// \param[in] storage_ storage to use
	/// \param[in] processor_ query processor to use
	/// \param[in] parameter_ parameter
	/// \param[in] errorhnd_ error buffer
	SummarizerFunctionContextMatchesBase(
			const StorageClientInterface* storage_,
			const MatchesBaseParameter& parameter_,
			const char* name_,
			ErrorBufferInterface* errorhnd_);

	virtual ~SummarizerFunctionContextMatchesBase(){}

	virtual void addSummarizationFeature(
			const std::string& name,
			PostingIteratorInterface* itr,
			const std::vector<SummarizationVariable>& variables,
			double weight,
			const TermStatistics&);

	virtual void setVariableValue( const std::string& name, double value);

	virtual std::vector<SummaryElement> getSummary( const strus::WeightedDocument& doc)=0;

protected:
	void start( const strus::WeightedDocument& doc);
	bool next();

	int currentIndex() const
	{
		return m_cur_featureidx;
	}

	const std::string& currentValue() const
	{
		return m_cur_value;
	}

	double currentWeight() const
	{
		return m_cur_weight;
	}

protected:
	struct SummarizationFeature
	{
		PostingIteratorInterface* itr;
		std::vector<const PostingIteratorInterface*> varitr;
		strus::NamedFormatString fmt;
		double weight;
		bool valid;

		SummarizationFeature(
				PostingIteratorInterface* itr_,
				const std::vector<const PostingIteratorInterface*>& varitr_,
				const strus::NamedFormatString& fmt_,
				double weight_)
			:itr(itr_),varitr(varitr_),fmt(fmt_),weight(weight_),valid(false){}
		SummarizationFeature( const SummarizationFeature& o)
			:itr(o.itr),varitr(o.varitr),fmt(o.fmt),weight(o.weight),valid(o.valid){}
	};

	MatchesBaseParameter m_parameter;				///< parameters
	const char* m_name;						///< summarizer name
	const StorageClientInterface* m_storage;			///< storage interface
	Reference<ForwardIteratorInterface> m_tag_forwardindex;		///< forward index interface for tag elements to collect
	Reference<ForwardIteratorInterface> m_text_forwardindex;	///< forward index interface for text elements to collect
	std::vector<SummarizationFeature> m_features;			///< features used for summarization
	int m_cur_featureidx;						///< current index into m_features
	strus::Index m_cur_pos;						///< current position of posting iterator
	strus::Index m_end_pos;						///< end position of the field to summarize on
	std::string m_cur_value;					///< current value
	double m_cur_weight;						///< current weight
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


/// \brief Base class for instance implementing a summarizer listing or accumulating matches
class SummarizerFunctionInstanceMatchesBase
	:public SummarizerFunctionInstanceInterface
{
public:
	SummarizerFunctionInstanceMatchesBase( const char* name_, ErrorBufferInterface* errorhnd_)
		:m_parameter(),m_name(name_),m_errorhnd(errorhnd_){}

	virtual ~SummarizerFunctionInstanceMatchesBase(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const NumericVariant& value);

	virtual std::vector<std::string> getVariables() const
	{
		return std::vector<std::string>();
	}

	virtual SummarizerFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage,
			const GlobalStatistics&) const=0;

	virtual bool doPopulate() const=0;

	virtual const char* name() const
	{
		return m_name;
	}
	virtual StructView view() const;

protected:
	MatchesBaseParameter m_parameter;		///< parameters
	const char* m_name;				///< summarizer name
	ErrorBufferInterface* m_errorhnd;		///< buffer for error messages
};


class SummarizerFunctionMatchesBase
	:public SummarizerFunctionInterface
{
public:
	explicit SummarizerFunctionMatchesBase( const char* name_, ErrorBufferInterface* errorhnd_)
		:m_name(name_),m_errorhnd(errorhnd_){}
	virtual ~SummarizerFunctionMatchesBase(){}

	virtual SummarizerFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface* processor) const=0;

	virtual const char* name() const
	{
		return m_name;
	}
	virtual StructView view() const;

protected:
	const char* m_name;				///< summarizer name
	ErrorBufferInterface* m_errorhnd;		///< buffer for error messages
};

}//namespace
#endif


