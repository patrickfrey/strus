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
#ifndef _STRUS_WEIGHTING_CONSTANT_HPP_INCLUDED
#define _STRUS_WEIGHTING_CONSTANT_HPP_INCLUDED
#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/weightingClosureInterface.hpp"
#include "strus/index.hpp"
#include "strus/arithmeticVariant.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/private/arithmeticVariantAsString.hpp"
#include "private/internationalization.hpp"
#include "private/utils.hpp"
#include <limits>
#include <vector>

namespace strus
{

/// \brief Forward declaration
class WeightingFunctionConstant;


/// \class WeightingClosureConstant
/// \brief Weighting function closure for the constant weighting function
class WeightingClosureConstant
	:public WeightingClosureInterface
{
public:
	WeightingClosureConstant(
			PostingIteratorInterface* itr_,
			float weight_)
		:m_itr(itr_),m_weight(weight_){}

	virtual float call( const Index& docno)
	{
		return docno==m_itr->skipDoc(docno)?(m_weight):(float)0.0;
	}

private:
	PostingIteratorInterface* m_itr;
	float m_weight;
};

/// \class WeightingFunctionInstanceConstant
/// \brief Weighting function instance for a weighting that returns a constant for every matching document
class WeightingFunctionInstanceConstant
	:public WeightingFunctionInstanceInterface
{
public:
	explicit WeightingFunctionInstanceConstant()
		:m_weight(1.0){}

	virtual ~WeightingFunctionInstanceConstant(){}

	virtual void addParameter( const std::string& name, const std::string& value)
	{
		addParameter( name, arithmeticVariantFromString( value));
	}

	virtual void addParameter( const std::string& name, const ArithmeticVariant& value)
	{
		if (utils::caseInsensitiveEquals( name, "weight"))
		{
			m_weight = (float)value;
		}
		else
		{
			throw strus::runtime_error( _TXT("unknown '%s' weighting function parameter '%s'"), "Constant", name.c_str());
		}
	}

	virtual WeightingClosureInterface* createClosure(
			const StorageClientInterface*,
			PostingIteratorInterface* itr,
			MetaDataReaderInterface*) const
	{
		return new WeightingClosureConstant( itr, m_weight);
	}

	virtual std::string tostring() const
	{
		std::ostringstream rt;
		rt << std::setw(2) << std::setprecision(5)
			<< "weight=" << m_weight;
		return rt.str();
	}

private:
	float m_weight;
};


/// \class WeightingFunctionConstant
/// \brief Weighting function that simply returns the ff (feature frequency in the document) multiplied with a constant weight 
class WeightingFunctionConstant
	:public WeightingFunctionInterface
{
public:
	WeightingFunctionConstant(){}
	virtual ~WeightingFunctionConstant(){}


	virtual WeightingFunctionInstanceInterface* createInstance() const
	{
		return new WeightingFunctionInstanceConstant();
	}
};

}//namespace
#endif

