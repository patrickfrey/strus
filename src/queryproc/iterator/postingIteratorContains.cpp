/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "postingIteratorContains.hpp"
#include "postingIteratorHelpers.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <stdexcept>
#include <cstdlib>

using namespace strus;

IteratorContains::IteratorContains(
		const std::vector<Reference< PostingIteratorInterface> >& args,
		ErrorBufferInterface* errorhnd_)
	:m_docno(0)
	,m_posno(0)
	,m_docnoAllMatchItr(args)
	,m_documentFrequency(-1)
	,m_errorhnd(errorhnd_)
{
	std::vector<Reference< PostingIteratorInterface> >::const_iterator
		ai = args.begin(), ae = args.end();
	for (int aidx=0; ai != ae; ++ai,++aidx)
	{
		if (aidx) m_featureid.push_back('=');
		m_featureid.append( (*ai)->featureid());
	}
	m_featureid.push_back( 'A');
}

IteratorContains::~IteratorContains()
{}

IteratorContainsWithCardinality::IteratorContainsWithCardinality(
		const std::vector<Reference< PostingIteratorInterface> >& args,
		unsigned int cardinality_,
		ErrorBufferInterface* errorhnd_)
	:m_docno(0)
	,m_posno(0)
	,m_prioqueue(args,cardinality_)
	,m_documentFrequency(-1)
	,m_errorhnd(errorhnd_)
{
	std::vector<Reference< PostingIteratorInterface> >::const_iterator
		ai = args.begin(), ae = args.end();
	for (int aidx=0; ai != ae; ++ai,++aidx)
	{
		if (aidx) m_featureid.push_back('=');
		m_featureid.append( (*ai)->featureid());
	}
	if (cardinality_)
	{
		encodeInteger( m_featureid, cardinality_);
		m_featureid.push_back( 'C');
	}
	m_featureid.push_back( 'A');
}

Index IteratorContains::skipDocCandidate( const Index& docno_)
{
	return m_docno = m_docnoAllMatchItr.skipDocCandidate( docno_);
}

Index IteratorContains::skipDoc( const Index& docno_)
{
	return m_docno = m_docnoAllMatchItr.skipDoc( docno_);
}

Index IteratorContainsWithCardinality::skipDocCandidate( const Index& docno_)
{
	return m_docno = m_prioqueue.skipDocCandidate( docno_);
}

Index IteratorContainsWithCardinality::skipDoc( const Index& docno_)
{
	return m_docno = m_prioqueue.skipDoc( docno_);
}


Index IteratorContains::documentFrequency() const
{
	if (m_documentFrequency < 0)
	{
		m_documentFrequency = m_docnoAllMatchItr.minDocumentFrequency();
	}
	return m_documentFrequency;
}

Index IteratorContainsWithCardinality::documentFrequency() const
{
	if (m_documentFrequency < 0)
	{
		m_documentFrequency = minDocumentFrequency( m_prioqueue.args());
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

PostingJoinOperatorInterface::Description PostingJoinContains::getDescription() const
{
	try
	{
		return Description( _TXT("Get the set of postings (d,1) for documents d that contain all of the argument features"));
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "contains", *m_errorhnd, Description());
}

