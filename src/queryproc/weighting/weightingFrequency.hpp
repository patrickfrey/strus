/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_WEIGHTING_TERM_FREQUENCY_HPP_INCLUDED
#define _STRUS_WEIGHTING_TERM_FREQUENCY_HPP_INCLUDED
#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/weightingFunctionContextInterface.hpp"
#include "strus/index.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "private/internationalization.hpp"
#include <limits>
#include <vector>

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;

/// \class WeightingFunctionContextTermFrequency
/// \brief Weighting function based on the TermFrequency formula
class WeightingFunctionContextTermFrequency
	:public WeightingFunctionContextInterface
{
public:
	explicit WeightingFunctionContextTermFrequency( ErrorBufferInterface* errorhnd_)
		:m_featar(),m_errorhnd(errorhnd_){}

	struct Feature
	{
		PostingIteratorInterface* itr;
		double weight;

		Feature( PostingIteratorInterface* itr_, double weight_)
			:itr(itr_),weight(weight_){}
		Feature( const Feature& o)
			:itr(o.itr),weight(o.weight){}
	};

	virtual void setVariableValue( const std::string& name_, double value);

	virtual void addWeightingFeature(
			const std::string& name_,
			PostingIteratorInterface* itr_,
			double weight_,
			const TermStatistics&);

	virtual double call( const Index& docno);

	virtual std::string debugCall( const Index& docno);

private:
	std::vector<Feature> m_featar;
	ErrorBufferInterface* m_errorhnd;	///< buffer for error messages
};


/// \class WeightingFunctionInstanceTermFrequency
/// \brief Weighting function instance based on the BM25 formula
class WeightingFunctionInstanceTermFrequency
	:public WeightingFunctionInstanceInterface
{
public:
	explicit WeightingFunctionInstanceTermFrequency( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}

	virtual ~WeightingFunctionInstanceTermFrequency(){}

	virtual void addStringParameter( const std::string& name_, const std::string& value);

	virtual void addNumericParameter( const std::string& name_, const NumericVariant&);

	virtual std::vector<std::string> getVariables() const
	{
		return std::vector<std::string>();
	}

	virtual WeightingFunctionContextInterface* createFunctionContext(
			const StorageClientInterface*,
			MetaDataReaderInterface*,
			const GlobalStatistics& stats_) const;

	virtual const char* name() const {return "tf";}
	virtual StructView view() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};



/// \class WeightingFunctionTermFrequency
/// \brief Weighting function that simply returns the ff (feature frequency in the document) multiplied with a constant weight 
class WeightingFunctionTermFrequency
	:public WeightingFunctionInterface
{
public:
	explicit WeightingFunctionTermFrequency( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	virtual ~WeightingFunctionTermFrequency(){}

	virtual WeightingFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface* processor) const;

	virtual const char* name() const {return "tf";}
	virtual StructView view() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif

