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
#ifndef _STRUS_STORAGE_BROWSE_POSTING_ITERATOR_HPP_INCLUDED
#define _STRUS_STORAGE_BROWSE_POSTING_ITERATOR_HPP_INCLUDED
#include "strus/postingIteratorInterface.hpp"
#include "strus/metaDataRestrictionInstanceInterface.hpp"
#include "strus/reference.hpp"
#include "strus/index.hpp"
#include <string>

namespace strus {

/// \brief Forward declaration
class MetaDataRestrictionInterface;

/// \brief Iterator representing an empty set
class BrowsePostingIterator
	:public PostingIteratorInterface
{
public:
	BrowsePostingIterator( 
		const MetaDataRestrictionInterface* restriction_,
		const Index& maxdocno_, const Index& maxposno_);

	virtual ~BrowsePostingIterator();

	virtual const char* featureid() const
	{
		return m_featureid.c_str();
	}

	virtual Index skipDoc( const Index& docno_);

	virtual Index skipDocCandidate( const Index& docno_)
	{
		if (docno_ < 0 || docno_ > m_maxdocno) return m_docno = 0;
		return m_docno = (docno_ == 0) ? 1 : docno_;
	}

	virtual Index skipPos( const Index& posno_)
	{
		if (posno_ < 0 || posno_ > m_maxposno) return m_posno = 0;
		if (posno_ == 0) return m_posno = 1;
		return m_posno = posno_;
	}

	virtual unsigned int frequency()
	{
		return m_maxposno;
	}

	virtual Index documentFrequency() const
	{
		return m_maxdocno;
	}
	
	virtual Index docno() const
	{
		return m_docno;
	}

	virtual Index posno() const
	{
		return m_posno;
	}

private:
	Reference<MetaDataRestrictionInstanceInterface> m_restriction;
	std::string m_featureid;
	Index m_docno;
	Index m_posno;
	Index m_maxdocno;
	Index m_maxposno;
};

}
#endif
