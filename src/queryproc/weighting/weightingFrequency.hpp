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
#include "strus/weightingExecutionContextInterface.hpp"
#include "strus/index.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/private/arithmeticVariantAsString.hpp"
#include "private/internationalization.hpp"
#include "private/utils.hpp"
#include <limits>
#include <vector>

namespace strus
{

/// \class WeightingExecutionContextTermFrequency
/// \brief Weighting function based on the TermFrequency formula
class WeightingExecutionContextTermFrequency
	:public WeightingExecutionContextInterface
{
public:
	explicit WeightingExecutionContextTermFrequency(
			PostingIteratorInterface* itr_)
		:m_itr(itr_){}

	virtual float call( const Index& docno)
	{
		return (docno==m_itr->skipDoc( docno)?(m_itr->frequency()):(float)0.0);
	}

private:
	PostingIteratorInterface* m_itr;
};


/// \class WeightingFunctionInstanceTermFrequency
/// \brief Weighting function instance based on the BM25 formula
class WeightingFunctionInstanceTermFrequency
	:public WeightingFunctionInstanceInterface
{
public:
	WeightingFunctionInstanceTermFrequency(){}

	virtual ~WeightingFunctionInstanceTermFrequency(){}

	virtual void addStringParameter( const std::string& name, const std::string& value)
	{
		addNumericParameter( name, arithmeticVariantFromString( value));
	}

	virtual void addNumericParameter( const std::string& name, const ArithmeticVariant&)
	{
		throw strus::runtime_error( _TXT("unknown '%s' weighting function parameter '%s'"), "BM25", name.c_str());
	}

	virtual WeightingExecutionContextInterface* createExecutionContext(
			const StorageClientInterface*,
			PostingIteratorInterface* itr,
			MetaDataReaderInterface*) const
	{
		return new WeightingExecutionContextTermFrequency( itr);
	}

	virtual std::string tostring() const
	{
		return std::string();
	}
};



/// \class WeightingFunctionTermFrequency
/// \brief Weighting function that simply returns the ff (feature frequency in the document) multiplied with a constant weight 
class WeightingFunctionTermFrequency
	:public WeightingFunctionInterface
{
public:
	WeightingFunctionTermFrequency(){}
	virtual ~WeightingFunctionTermFrequency(){}

	virtual WeightingFunctionInstanceInterface* createInstance() const
	{
		return new WeightingFunctionInstanceTermFrequency();
	}
};

}//namespace
#endif

