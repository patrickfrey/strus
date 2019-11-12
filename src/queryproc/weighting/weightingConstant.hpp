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
#include <limits>
#include <vector>
#include <map>

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
			double weight_, bool precalc_, ErrorBufferInterface* errorhnd_)
		:m_featar(),m_weight(weight_),m_precalc(precalc_),m_errorhnd(errorhnd_){}

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
			const TermStatistics& stats_);

	virtual double call( const Index& docno);

	virtual std::string debugCall( const Index& docno);

private:
	std::vector<Feature> m_featar;
	float m_weight;
	bool m_precalc;
	std::map<Index,double> m_precalcmap;
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

/// \class WeightingFunctionInstanceConstant
/// \brief Weighting function instance for a weighting that returns a constant for every matching document
class WeightingFunctionInstanceConstant
	:public WeightingFunctionInstanceInterface
{
public:
	explicit WeightingFunctionInstanceConstant( ErrorBufferInterface* errorhnd_)
		:m_weight(1.0),m_precalc(false),m_errorhnd(errorhnd_){}

	virtual ~WeightingFunctionInstanceConstant(){}

	virtual void addStringParameter( const std::string& name_, const std::string& value);

	virtual void addNumericParameter( const std::string& name_, const NumericVariant& value);

	virtual std::vector<std::string> getVariables() const
	{
		return std::vector<std::string>();
	}

	virtual WeightingFunctionContextInterface* createFunctionContext(
			const StorageClientInterface*,
			const GlobalStatistics&) const;

	virtual const char* name() const {return "constant";}
	virtual StructView view() const;

private:
	float m_weight;
	bool m_precalc;
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

	virtual const char* name() const {return "constant";}
	virtual StructView view() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif

