/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
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



