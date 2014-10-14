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
#ifndef _STRUS_ACCUMULATOR_OKAPI_HPP_INCLUDED
#define _STRUS_ACCUMULATOR_OKAPI_HPP_INCLUDED
#include "strus/accumulatorInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/index.hpp"
#include "iteratorReference.hpp"
#include <vector>

namespace strus
{

/// \class AccumulatorOkapi (BM25)
/// \brief Accumulator using the BM25 weighting formula
class AccumulatorOkapi
	:public AccumulatorInterface
{
public:
	explicit AccumulatorOkapi(
			const StorageInterface* storage_,
			const IteratorInterface& itr_,
			float b_,
			float k1_,
			float avgDocLength_,
			Index estimatedNofMatches_);

	AccumulatorOkapi( const AccumulatorOkapi& o);

	virtual ~AccumulatorFrequency(){}

	virtual AccumulatorInterface* copy() const;

	virtual Index skipDoc( const Index& docno_);

	virtual double weight();

private:
	const StorageInterface* m_storage;
	Index m_docno;
	IteratorReference m_itr;		///< input occurrencies to scan for results
	float m_b;
	float m_k1;
	float m_avgDocLength;
	Index m_estimatedNofMatches;
	double m_idf;
};

}//namespace
#endif

