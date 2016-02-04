/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#ifndef _STRUS_WEIGHTING_METADATA_HPP_INCLUDED
#define _STRUS_WEIGHTING_METADATA_HPP_INCLUDED
#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/weightingFunctionContextInterface.hpp"
#include "strus/index.hpp"
#include "strus/arithmeticVariant.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "private/internationalization.hpp"
#include "private/utils.hpp"
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
			float weight_,
			ErrorBufferInterface* errorhnd_);

	virtual void addWeightingFeature(
			const std::string&,
			PostingIteratorInterface*,
			float/*weight*/,
			const TermStatistics&);

	virtual float call( const Index& docno);

private:
	MetaDataReaderInterface* m_metadata;
	Index m_elementHandle;
	float m_weight;
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
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

	virtual void addStringParameter( const std::string& name, const std::string& value);

	virtual void addNumericParameter( const std::string& name, const ArithmeticVariant& value);

	virtual void addBooleanParameter( const std::string& name, const bool& value);

	virtual WeightingFunctionContextInterface* createFunctionContext(
			const StorageClientInterface*,
			MetaDataReaderInterface* metadata_,
			const GlobalStatistics&) const;

	virtual std::string tostring() const;

private:
	float m_weight;
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

	virtual WeightingFunctionInstanceInterface* createInstance() const;

	virtual Description getDescription() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif

