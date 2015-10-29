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
#include "postingIteratorContains.hpp"
#include "postingIteratorHelpers.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <stdexcept>
#include <cstdlib>

using namespace strus;

IteratorContains::IteratorContains( const std::vector<Reference< PostingIteratorInterface> >& args, ErrorBufferInterface* errorhnd_)
	:m_docno(0)
	,m_posno(0)
	,m_argar(args)
	,m_documentFrequency(-1)
	,m_errorhnd(errorhnd_)
{
	std::vector<Reference< PostingIteratorInterface> >::const_iterator
		ai = m_argar.begin(), ae = m_argar.end();
	for (int aidx=0; ai != ae; ++ai,++aidx)
	{
		if (aidx) m_featureid.push_back('=');
		m_featureid.append( (*ai)->featureid());
	}
	m_featureid.push_back( 'A');
}

IteratorContains::~IteratorContains()
{}

IteratorContainsWithCardinality::IteratorContainsWithCardinality( const std::vector<Reference< PostingIteratorInterface> >& args, std::size_t cardinality_, ErrorBufferInterface* errorhnd_)
	:IteratorContains(args,errorhnd_), m_cardinality(cardinality_)
{
	m_featureid.resize( m_featureid.size()-1);
	if (m_cardinality && m_cardinality != args.size())
	{
		encodeInteger( m_featureid, m_cardinality);
		m_featureid.push_back( 'C');
	}
	m_featureid.push_back( 'A');
}

std::vector<const PostingIteratorInterface*>
	IteratorContains::subExpressions( bool positive) const
{
	try
	{
		std::vector<const PostingIteratorInterface*> rt;
		if (positive)
		{
			rt.reserve( m_argar.size());
			std::vector<Reference< PostingIteratorInterface> >::const_iterator
				ai = m_argar.begin(), ae = m_argar.end();
			for (; ai != ae; ++ai)
			{
				rt.push_back( ai->get());
			}
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error '%s' iterator getting subexpressions: %s"), "contains", *m_errorhnd, std::vector<const PostingIteratorInterface*>());
}

Index IteratorContains::skipDoc( const Index& docno_)
{
	if (m_docno == docno_ && m_docno) return m_docno;
	return m_docno = getFirstAllMatchDocno( m_argar, docno_);
}

Index IteratorContainsWithCardinality::skipDoc( const Index& docno_)
{
	if (m_docno == docno_ && m_docno) return m_docno;
	return m_docno = getFirstAllMatchDocnoSubset( m_argar, docno_, m_cardinality);
}

GlobalCounter IteratorContains::documentFrequency() const
{
	if (m_documentFrequency < 0)
	{
		std::vector<Reference< PostingIteratorInterface> >::const_iterator
			ai = m_argar.begin(), ae = m_argar.end();
		if (ai == ae) return 0;

		m_documentFrequency = (*ai)->documentFrequency();
		for (++ai; ai != ae && m_documentFrequency < 0; ++ai)
		{
			Index df = (*ai)->documentFrequency();
			if (df < m_documentFrequency)
			{
				m_documentFrequency = df;
			}
		}
	}
	return m_documentFrequency;
}


PostingIteratorInterface* PostingJoinContains::createResultIterator(
		const std::vector<Reference<PostingIteratorInterface> >& itrs,
		int range,
		unsigned int cardinality) const
{
	if (cardinality > itrs.size())
	{
		m_errorhnd->report( _TXT( "cardinality of 'contains' is out of range"));
		return 0;
	}
	if (range != 0)
	{
		m_errorhnd->report( _TXT( "no range argument expected for 'contains'"));
		return 0;
	}
	if (itrs.size() == 0)
	{
		m_errorhnd->report( _TXT( "too few arguments for 'contains'"));
		return 0;
	}
	try
	{
		if (cardinality == 0 || cardinality == itrs.size())
		{
			return new IteratorContains( itrs, m_errorhnd);
		}
		else
		{
			return new IteratorContainsWithCardinality( itrs, cardinality, m_errorhnd);
		}
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "contains", *m_errorhnd, 0);
}
