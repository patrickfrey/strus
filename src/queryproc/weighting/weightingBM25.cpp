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
#include "weightingBM25.hpp"
#include "strus/constants.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include <cmath>

using namespace strus;

WeightingBM25::WeightingBM25(
	const StorageInterface* storage_,
	float k1_,
	float b_,
	float avgDocLength_)
		:WeightingIdfBased(storage_)
		,m_storage(storage_)
		,m_doclenReader( storage_->createMetaDataReader( Constants::DOC_ATTRIBUTE_DOCLEN))
		,m_k1(k1_)
		,m_b(b_)
		,m_avgDocLength(avgDocLength_)
		,m_docno(0)
		
{}

WeightingBM25::WeightingBM25( const WeightingBM25& o)
	:WeightingIdfBased(o)
	,m_storage(o.m_storage)
	,m_k1(o.m_k1)
	,m_b(o.m_b)
	,m_avgDocLength(o.m_avgDocLength)
	,m_docno(o.m_docno)
{}

WeightingBM25::~WeightingBM25()
{
	delete m_doclenReader;
}

float WeightingBM25::call( IteratorInterface& itr)
{
	if (!idf_calculated())
	{
		calculateIdf( itr);
	}
	m_docno = itr.docno();
	float rel_doclen = (m_doclenReader->readValue( m_docno)+1) / m_avgDocLength;
	float ff = itr.frequency();
	if (ff == 0.0)
	{
		return 0.0;
	}
	else
	{
		return idf()
			* (ff * (m_k1 + 1.0))
			/ (ff + m_k1 * (1.0 - m_b + m_b * rel_doclen));
	}
}



