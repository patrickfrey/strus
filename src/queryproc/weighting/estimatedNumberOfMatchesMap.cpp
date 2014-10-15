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
#include "estimatedNumberOfMatchesMap.hpp"

using namespace strus;

EstimatedNumberOfMatchesMap::EstimatedNumberOfMatchesMap( const StorageInterface* storage_)
	:m_storage(storage_)
	,m_maxDocumentNumber( storage_->maxDocumentNumber())
	,m_nofDocumentsInCollection( storage_->nofDocumentsInserted())
{}

static Index ror(Index x, unsigned int moves)
{
	return (x >> moves) | (x << (sizeof(Index)*8 - moves));
}

static Index hash64shift( Index key)
{
	key = (~key) + (key << 21);
	key = key ^ ror( key, 24);
	key = (key + (key << 3)) + (key << 8); // key * 265
	key = key ^ ror(key, 14);
	key = (key + (key << 2)) + (key << 4); // key * 21
	key = key ^ ror( key, 28);
	key = key + (key << 31);
	return key;
}

static Index randomDocumentNumber( Index maxdocno, unsigned int no)
{
	return hash64shift( maxdocno + (no * 2654435761U)) % (maxdocno+1);
}

static double estimateNumberOfMatches(
		IteratorInterface& itr,
		Index maxDocumentNumber,
		Index nofDocumentsInCollection)
{
	enum {Iterations=10, LocalScans=5};
	double diffsum = 0.0;

	if (itr.skipDoc(0) == 0)
	{
		return 0.0;
	}
	unsigned int ii = 0;
	for (; ii<Iterations; ++ii)
	{
		Index pick_docno = randomDocumentNumber( maxDocumentNumber, ii);
		unsigned int kk = 0;
		for (; kk<LocalScans; ++kk)
		{
			Index next_docno = itr.skipDoc( pick_docno);
			if (next_docno == 0)
			{
				next_docno = itr.skipDoc(0) + maxDocumentNumber;
			}
			diffsum += next_docno - pick_docno + 1;
			if (next_docno == 0)
			{
				pick_docno = randomDocumentNumber( maxDocumentNumber, ii * Iterations + kk);
			}
			else
			{
				pick_docno = next_docno+1;
			}
		}
	}
	return (double) (nofDocumentsInCollection / diffsum) * Iterations * LocalScans;
}


double EstimatedNumberOfMatchesMap::get( IteratorInterface& itr)
{
	std::map<std::string,double>::const_iterator vi = m_valmap.find( itr.featureid());
	if (vi != m_valmap.end())
	{
		return vi->second;
	}
	else
	{
		double val = estimateNumberOfMatches(
				itr, m_maxDocumentNumber,
				m_nofDocumentsInCollection);
		m_valmap[ itr.featureid()] = val;
		return val;
	}
}

