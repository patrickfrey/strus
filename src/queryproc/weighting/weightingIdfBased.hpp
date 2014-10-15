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
#ifndef _STRUS_WEIGHTING_IDF_BASED_HPP_INCLUDED
#define _STRUS_WEIGHTING_IDF_BASED_HPP_INCLUDED
#include "strus/weightingFunctionInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/index.hpp"
#include "iteratorReference.hpp"
#include "weighting/estimatedNumberOfMatchesMap.hpp"
#include <vector>

namespace strus
{

/// \class WeightingIdfBased
/// \brief Interface for weighting function based on IDF
class WeightingIdfBased
	:public WeightingFunctionInterface
{
public:
	WeightingIdfBased(
			const StorageInterface* storage_,
			const EstimatedNumberOfMatchesMapR& nofMatchesMap_)
		:m_storage(storage_)
		,m_nofMatchesMap(nofMatchesMap_)
		,m_idf_calculated(false)
		,m_idf(0.0){}

	WeightingIdfBased( const WeightingIdfBased& o)
		:m_storage(o.m_storage)
		,m_nofMatchesMap(o.m_nofMatchesMap)
		,m_idf_calculated(o.m_idf_calculated)
		,m_idf(o.m_idf){}

	virtual ~WeightingIdfBased(){}

	virtual WeightingFunctionInterface* copy() const=0;

	virtual double call( IteratorInterface& itr)=0;

protected:
	bool idf_calculated() const			{return m_idf_calculated;}
	double idf() const				{return m_idf;}
	void calculateIdf( IteratorInterface& itr);

private:
	const StorageInterface* m_storage;
	EstimatedNumberOfMatchesMapR m_nofMatchesMap;
	bool m_idf_calculated;
	double m_idf;
};

}//namespace
#endif

