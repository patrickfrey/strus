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
#include "strus/weightingClosureInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/index.hpp"
#include "postingIteratorReference.hpp"
#include <vector>

namespace strus
{

/// \brief Forward declaration
class WeightingFunctionBM25;


/// \class WeightingClosureBM25
/// \brief Weighting function based on the BM25 formula
class WeightingClosureBM25
	:public WeightingClosureInterface
{
public:
	WeightingClosureBM25(
		const StorageInterface* storage,
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

/// \class WeightingFunctionBM25
/// \brief Weighting function based on the BM25 formula
class WeightingFunctionBM25
	:public WeightingFunctionInterface
{
public:
	explicit WeightingFunctionBM25(){}

	virtual ~WeightingFunctionBM25(){}

	virtual const char** parameterNames() const
	{
		static const char* ar[] = {"k1","b","avgdoclen",0};
		return ar;
	}

	virtual WeightingClosureInterface* createClosure(
			const StorageInterface* storage_,
			PostingIteratorInterface* itr,
			MetaDataReaderInterface* metadata,
			const std::vector<ArithmeticVariant>& parameters) const
	{
		float b  = parameters[0].defined()?(float)parameters[0]:0.75;
		float k1 = parameters[1].defined()?(float)parameters[1]:1.5;
		float al = parameters[2].defined()?(float)parameters[2]:1000;

		return new WeightingClosureBM25( storage_, itr, metadata, b, k1, al);
	}
};

}//namespace
#endif

