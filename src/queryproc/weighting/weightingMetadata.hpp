/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_WEIGHTING_METADATA_HPP_INCLUDED
#define _STRUS_WEIGHTING_METADATA_HPP_INCLUDED
#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/weightingFunctionContextInterface.hpp"
#include "strus/index.hpp"
#include "strus/numericVariant.hpp"
#include "strus/reference.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "private/internationalization.hpp"
#include <limits>
#include <vector>

namespace strus
{

/// \brief Forward declaration
class WeightingFunctionMetadata;
/// \brief Forward declaration
class ErrorBufferInterface;


/// \class WeightingFunctionContextMetadata
/// \brief Weighting function FunctionContext for the metadata weighting function
class WeightingFunctionContextMetadata
	:public WeightingFunctionContextInterface
{
public:
	WeightingFunctionContextMetadata(
			MetaDataReaderInterface* metadata_,
			const std::string& elementName_,
			double weight_,
			ErrorBufferInterface* errorhnd_);

	virtual void setVariableValue( const std::string& name_, double value);

	virtual void addWeightingFeature(
			const std::string&,
			PostingIteratorInterface*,
			double/*weight*/,
			const TermStatistics&);

	virtual const std::vector<WeightedField>& call( const Index& docno);

private:
	strus::Reference<MetaDataReaderInterface> m_metadata;
	Index m_elementHandle;
	double m_weight;
	std::vector<WeightedField> m_lastResult;	///< buffer for the last result calculated
	ErrorBufferInterface* m_errorhnd;		///< buffer for error messages
};

/// \class WeightingFunctionInstanceMetadata
/// \brief Weighting function instance for a weighting that returns a metadata element for every matching document
class WeightingFunctionInstanceMetadata
	:public WeightingFunctionInstanceInterface
{
public:
	explicit WeightingFunctionInstanceMetadata( ErrorBufferInterface* errorhnd_)
		:m_weight(1.0),m_errorhnd(errorhnd_){}

	virtual ~WeightingFunctionInstanceMetadata(){}

	virtual void addStringParameter( const std::string& name_, const std::string& value);

	virtual void addNumericParameter( const std::string& name_, const NumericVariant& value);

	virtual std::vector<std::string> getVariables() const
	{
		return std::vector<std::string>();
	}

	virtual WeightingFunctionContextInterface* createFunctionContext(
			const StorageClientInterface*,
			const GlobalStatistics&) const;

	virtual const char* name() const	{return "metadata";}
	virtual StructView view() const;

private:
	double m_weight;
	std::string m_elementName;
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


/// \class WeightingFunctionMetadata
/// \brief Weighting function that simply returns the value of a meta data element multiplied by a weight
class WeightingFunctionMetadata
	:public WeightingFunctionInterface
{
public:
	explicit WeightingFunctionMetadata( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	virtual ~WeightingFunctionMetadata(){}

	virtual WeightingFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface* processor) const;

	virtual const char* name() const	{return "metadata";}
	virtual StructView view() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif

