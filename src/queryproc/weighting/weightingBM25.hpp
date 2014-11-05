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
#include "strus/storageInterface.hpp"
#include "strus/index.hpp"
#include "iteratorReference.hpp"
#include "weightingIdfBased.hpp"
#include <vector>
#include <boost/shared_ptr.hpp>

namespace strus
{

/// \class WeightingBM25
/// \brief Weighting function based on the BM25 formula
class WeightingBM25
	:public WeightingIdfBased
{
public:
	explicit WeightingBM25(
			const StorageInterface* storage_,
			float k1_,
			float b_,
			float avgDocLength_);

	WeightingBM25( const WeightingBM25& o);

	virtual ~WeightingBM25(){}

	virtual float call( IteratorInterface& itr);

private:
	const StorageInterface* m_storage;
	float m_k1;
	float m_b;
	float m_avgDocLength;
};

}//namespace
#endif

