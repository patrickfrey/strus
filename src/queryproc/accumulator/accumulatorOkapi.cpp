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
#include "accumulatorOkapi.hpp"
#include <cmath>

using namespace strus;

AccumulatorOkapi::AccumulatorOkapi(
		const StorageInterface* storage_,
		const IteratorInterface& itr_,
		float b_,
		float k1_,
		float avgDocLength_,
		Index estimatedNofMatches_)
	:m_storage(storage_)
	,m_docno(0)
	,m_itr(itr_.copy())
	,m_b(b_)
	,m_k1(k1_)
	,m_avgDocLength(avgDocLength_)
	,m_estimatedNofMatches(estimatedNofMatches_)
{
	Index nofCollectionDocuments = nofDocumentsInserted();
	if (nofCollectionDocuments > m_estimatedNofMatches * 2)
	{
		m_idf = log(
			(double)(nofCollectionDocuments_ - m_estimatedNofMatches + 0.5)
			/ (m_estimatedNofMatches + 0.5));
	}
	else
	{
		m_idf = 0.0;
	}
}

AccumulatorOkapi::AccumulatorOkapi( const AccumulatorOkapi& o)
	:m_storage(o.m_storage)
	,m_docno(o.m_docno)
	,m_itr(o.m_itr)
	,m_b(o.m_b)
	,m_k1(o.m_k1)
	,m_avgDocLength(o.m_avgDocLength)
	,m_estimatedNofMatches(o.m_estimatedNofMatches)
	,m_idf(o.m_idf)
{}

AccumulatorInterface* AccumulatorOkapi::copy() const
{
	return new AccumulatorOkapi( *m_itr);
}

Index AccumulatorOkapi::skipDoc( const Index& docno_)
{
	m_docno = m_itr->skipDoc( docno_);
	return m_docno;
}

double AccumulatorOkapi::weight()
{
	double relativeDocLen
		= (double)m_storage->documentAttributeNumeric( m_docno, 'D');
		/ m_avgDocLength;
	return IDF
		* ((double)m_itr->frequency() * (m_k1 + 1.0))
		/ ((double)m_itr->frequency() + m_k1 * (1.0 - m_b + m_b * relativeDocLen));
}

