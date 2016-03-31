/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_WEIGHTING_CONSTANT_HPP_INCLUDED
#define _STRUS_WEIGHTING_CONSTANT_HPP_INCLUDED
#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/weightingFunctionContextInterface.hpp"
#include "strus/index.hpp"
#include "strus/numericVariant.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "private/internationalization.hpp"
#include "private/utils.hpp"
#include <limits>
#include <vector>

namespace strus
{

/// \brief Forward declaration
class WeightingFunctionConstant;
/// \brief Forward declaration
class ErrorBufferInterface;


/// \class WeightingFunctionContextConstant
/// \brief Weighting function FunctionContext for the constant weighting function
class WeightingFunctionContextConstant
	:public WeightingFunctionContextInterface
{
public:
	WeightingFunctionContextConstant(
			float weight_, ErrorBufferInterface* errorhnd_)
		:m_featar(),m_weight(weight_),m_errorhnd(errorhnd_){}

	struct Feature
	{
		PostingIteratorInterface* itr;
		float weight;

		Feature( PostingIteratorInterface* itr_, float weight_)
			:itr(itr_),weight(weight_){}
		Feature( const Feature& o)
			:itr(o.itr),weight(o.weight){}
	};

	virtual void addWeightingFeature(
			const std::string& name_,
			PostingIteratorInterface* itr_,
			float weight_,
			const TermStatistics& stats_);

	virtual double call( const Index& docno);

private:
	std::vector<Feature> m_featar;
	float m_weight;
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

/// \class WeightingFunctionInstanceConstant
/// \brief Weighting function instance for a weighting that returns a constant for every matching document
class WeightingFunctionInstanceConstant
	:public WeightingFunctionInstanceInterface
{
public:
	explicit WeightingFunctionInstanceConstant( ErrorBufferInterface* errorhnd_)
		:m_weight(1.0),m_errorhnd(errorhnd_){}

	virtual ~WeightingFunctionInstanceConstant(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);

	virtual void addNumericParameter( const std::string& name, const NumericVariant& value);

	virtual WeightingFunctionContextInterface* createFunctionContext(
			const StorageClientInterface*,
			MetaDataReaderInterface*,
			const GlobalStatistics&) const;

	virtual std::string tostring() const;

private:
	float m_weight;
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


/// \class WeightingFunctionConstant
/// \brief Weighting function that simply returns a constant weight for every match
class WeightingFunctionConstant
	:public WeightingFunctionInterface
{
public:
	explicit WeightingFunctionConstant( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	virtual ~WeightingFunctionConstant(){}


	virtual WeightingFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface* processor) const;

	virtual FunctionDescription getDescription() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif

