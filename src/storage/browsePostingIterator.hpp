/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_BROWSE_POSTING_ITERATOR_HPP_INCLUDED
#define _STRUS_STORAGE_BROWSE_POSTING_ITERATOR_HPP_INCLUDED
#include "strus/postingIteratorInterface.hpp"
#include "strus/metaDataRestrictionInstanceInterface.hpp"
#include "strus/reference.hpp"
#include "strus/storage/index.hpp"
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

	virtual int frequency()
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

	virtual Index length() const
	{
		return m_posno?1:0;
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
