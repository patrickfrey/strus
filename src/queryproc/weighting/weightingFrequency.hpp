/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#ifndef _STRUS_WEIGHTING_TERM_FREQUENCY_HPP_INCLUDED
#define _STRUS_WEIGHTING_TERM_FREQUENCY_HPP_INCLUDED
#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/weightingFunctionContextInterface.hpp"
#include "strus/index.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "private/internationalization.hpp"
#include "private/utils.hpp"
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
		float weight;

		Feature( PostingIteratorInterface* itr_, float weight_)
			:itr(itr_),weight(weight_){}
		Feature( const Feature& o)
			:itr(o.itr),weight(o.weight){}
	};

	virtual void addWeightingFeature(
			const std::string& name_,
			PostingIteratorInterface* itr_,
			float weight_);

	virtual float call( const Index& docno);

private:
	std::vector<Feature> m_featar;
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
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

	virtual void addStringParameter( const std::string& name, const std::string& value);

	virtual void addNumericParameter( const std::string& name, const ArithmeticVariant&);

	virtual WeightingFunctionContextInterface* createFunctionContext(
			const StorageClientInterface*,
			MetaDataReaderInterface*,
			const GlobalStatistics&) const;

	virtual std::string tostring() const;

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

	virtual WeightingFunctionInstanceInterface* createInstance() const;

	virtual const char* getDescription() const
	{
		return _TXT("Calculate the weight of a document as sum of the feature frequency of a feature multiplied with its weight of the feature. The weighted features are defined with the feature parameter 'match'.");
	}

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif

