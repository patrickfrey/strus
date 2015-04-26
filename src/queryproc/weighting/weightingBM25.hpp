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
#ifndef _STRUS_WEIGHTING_BM25_HPP_INCLUDED
#define _STRUS_WEIGHTING_BM25_HPP_INCLUDED
#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/weightingExecutionContextInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/index.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/private/arithmeticVariantAsString.hpp"
#include "private/internationalization.hpp"
#include "private/utils.hpp"
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>

namespace strus
{

/// \brief Forward declaration
class WeightingFunctionBM25;


/// \class WeightingExecutionContextBM25
/// \brief Weighting function based on the BM25 formula
class WeightingExecutionContextBM25
	:public WeightingExecutionContextInterface
{
public:
	WeightingExecutionContextBM25(
		const StorageClientInterface* storage,
		PostingIteratorInterface* itr_,
		MetaDataReaderInterface* metadata_,
		float k1_,
		float b_,
		float avgDocLength_);

	virtual float call( const Index& docno);

private:
	float m_k1;
	float m_b;
	float m_avgDocLength;
	PostingIteratorInterface* m_itr;
	MetaDataReaderInterface* m_metadata;
	int m_metadata_doclen;
	float m_idf;
};


/// \class WeightingFunctionInstanceBM25
/// \brief Weighting function instance based on the BM25 formula
class WeightingFunctionInstanceBM25
	:public WeightingFunctionInstanceInterface
{
public:
	explicit WeightingFunctionInstanceBM25()
		:m_b(0.75),m_k1(1.5),m_avgdoclen(1000){}

	virtual ~WeightingFunctionInstanceBM25(){}

	virtual void addStringParameter( const std::string& name, const std::string& value)
	{
		addNumericParameter( name, arithmeticVariantFromString( value));
	}

	virtual void addNumericParameter( const std::string& name, const ArithmeticVariant& value)
	{
		if (utils::caseInsensitiveEquals( name, "k1"))
		{
			m_k1 = (float)value;
		}
		else if (utils::caseInsensitiveEquals( name, "b"))
		{
			m_b = (float)value;
		}
		else if (utils::caseInsensitiveEquals( name, "avgdoclen"))
		{
			m_avgdoclen = (float)value;
		}
		else
		{
			throw strus::runtime_error( _TXT("unknown '%s' weighting function parameter '%s'"), "BM25", name.c_str());
		}
	}

	virtual WeightingExecutionContextInterface* createExecutionContext(
			const StorageClientInterface* storage_,
			PostingIteratorInterface* itr,
			MetaDataReaderInterface* metadata) const
	{
		return new WeightingExecutionContextBM25( storage_, itr, metadata, m_b, m_k1, m_avgdoclen);
	}

	virtual std::string tostring() const
	{
		std::ostringstream rt;
		rt << std::setw(2) << std::setprecision(5)
			<< "b=" << m_b << ", k1=" << m_k1 << ", avgdoclen=" << m_avgdoclen;
		return rt.str();
	}

private:
	float m_b;
	float m_k1;
	float m_avgdoclen;
};


/// \class WeightingFunctionBM25
/// \brief Weighting function based on the BM25 formula
class WeightingFunctionBM25
	:public WeightingFunctionInterface
{
public:
	WeightingFunctionBM25(){}

	virtual ~WeightingFunctionBM25(){}

	virtual WeightingFunctionInstanceInterface* createInstance() const
	{
		return new WeightingFunctionInstanceBM25();
	}
};

}//namespace
#endif

