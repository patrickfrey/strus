/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "postingIteratorBetween.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"

using namespace strus;

#define THIS_ITERATOR_NAME const_cast<char*>("between")

IteratorBetween::IteratorBetween(
		const std::vector<Reference< PostingIteratorInterface> >& args,
		ErrorBufferInterface* errorhnd_)
	:m_docno(0)
	,m_posno(0)
	,m_field()
	,m_docnoAllMatchItr(args)
	,m_featureid()
	,m_documentFrequency(-1)
	,m_errorhnd(errorhnd_)
{
	if (args.size() > 3) throw strus::runtime_error(_TXT("too many arguments for '%s' iterator"), THIS_ITERATOR_NAME);
	if (args.size() < 2) throw strus::runtime_error(_TXT("too few arguments for '%s' iterator"), THIS_ITERATOR_NAME);

	m_elemitr = args[0];
	m_startitr = args[1];
	m_enditr = (args.size() >= 3) ? args[2] : args[1];

	m_featureid.append( m_elemitr->featureid());
	m_featureid.push_back('=');
	m_featureid.append( m_startitr->featureid());
	m_featureid.push_back('=');
	m_featureid.append( m_enditr->featureid());
	m_featureid.push_back( 'B');
}


Index IteratorBetween::skipDocCandidate( const Index& docno_)
{
	return m_docno = m_docnoAllMatchItr.skipDocCandidate( docno_);
}

Index IteratorBetween::skipDoc( const Index& docno_)
{
	m_docno = m_docnoAllMatchItr.skipDocCandidate( docno_);
	while (m_docno)
	{
		if (skipPos(0)) return m_docno;
		m_docno = m_docnoAllMatchItr.skipDocCandidate( m_docno+1);
	}
	return m_docno;
}

Index IteratorBetween::skipPos( const Index& pos_)
{
	if (!m_docno) return m_posno=0;
	if (m_field.contain(pos_))
	{
		m_posno = m_elemitr->skipPos( pos_);
		if (m_posno < m_field.end()) return m_posno;
	}
	Index pp = pos_;
	for (;;)
	{
		strus::Index start = m_startitr->skipPos( pp);
		if (!start) return m_posno = 0;
		strus::Index end = m_enditr->skipPos( start+1);
		if (!end) return m_posno = 0;
		strus::Index elem = m_startitr->skipPos( start);
		if (!elem) return m_posno = 0;
		if (elem < end)
		{
			m_field.init( start, end);
			return m_posno = elem;
		}
		else
		{
			pp = elem+1;
		}
	}
}

GlobalCounter IteratorBetween::documentFrequency() const
{
	if (m_documentFrequency < 0)
	{
		strus::Index df_start = m_startitr->documentFrequency();
		strus::Index df_end = m_enditr->documentFrequency();
		strus::Index df_elem  = m_elemitr->documentFrequency();
		return df_start < df_end
				? df_start < df_elem
					? df_start
					: df_elem
				: df_end < df_elem
					? df_end
					: df_elem
				;
	}
	return m_documentFrequency;
}


PostingIteratorInterface* PostingJoinBetween::createResultIterator(
		const std::vector<Reference<PostingIteratorInterface> >& itrs,
		int range,
		unsigned int cardinality) const
{
	if (range != 0)
	{
		m_errorhnd->report( ErrorCodeNotImplemented, _TXT( "no range argument expected for '%s'"), THIS_ITERATOR_NAME);
		return 0;
	}
	if (itrs.size() < 2)
	{
		m_errorhnd->report( ErrorCodeIncompleteDefinition, _TXT( "too few arguments for '%s'"), THIS_ITERATOR_NAME);
		return 0;
	}
	else if (itrs.size() > 3)
	{
		m_errorhnd->report( ErrorCodeIncompleteDefinition, _TXT( "too many arguments for '%s'"), THIS_ITERATOR_NAME);
		return 0;
	}
	try
	{
		return new IteratorBetween( itrs, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), THIS_ITERATOR_NAME, *m_errorhnd, 0);
}


StructView PostingJoinBetween::view() const
{
	try
	{
		return Description( THIS_ITERATOR_NAME, _TXT("Get the postings from the first argument iterator that are between the second argument iterator and the third argument iterator"));
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "intersect", *m_errorhnd, StructView());
}

const char* PostingJoinBetween::name() const
{
	return THIS_ITERATOR_NAME;
}
