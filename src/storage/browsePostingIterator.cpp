/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "browsePostingIterator.hpp"
#include "strus/metaDataRestrictionInterface.hpp"

using namespace strus;

BrowsePostingIterator::BrowsePostingIterator( const MetaDataRestrictionInterface* restriction_, const Index& maxdocno_, const Index& maxposno_)
	:m_restriction(restriction_->createInstance()),m_docno(0),m_posno(0),m_maxdocno(maxdocno_),m_maxposno(maxposno_)
{
#ifdef STRUS_LOWLEVEL_DEBUG
	m_featureid.append( m_restriction.get()?m_restriction->tostring():"");
#else
	m_featureid.append( "!BRO");
#endif
}

BrowsePostingIterator::~BrowsePostingIterator()
{}

Index BrowsePostingIterator::skipDoc( const Index& docno_)
{
	if (docno_ < 0 || docno_ > m_maxdocno) return m_docno = 0;
	Index dn = (docno_ == 0) ? 1 : docno_;
	while (!m_restriction->match( dn))
	{
		++dn;
		if (dn > m_maxdocno) return m_docno = 0;
	}
	return m_docno = dn;
}



