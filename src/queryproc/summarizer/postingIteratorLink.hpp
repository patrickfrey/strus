/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_POSTING_ITERATOR_LINK_HPP_INCLUDED
#define _STRUS_POSTING_ITERATOR_LINK_HPP_INCLUDED
#include "strus/postingIteratorInterface.hpp"

namespace strus
{

class PostingIteratorLink
	:public PostingIteratorInterface
{
public:
	PostingIteratorLink( PostingIteratorInterface* ref_)
		:m_ref(ref_){}

	virtual Index skipDoc( const Index& docno_)
	{
		return m_ref->skipDoc( docno_);
	}

	virtual Index skipDocCandidate( const Index& docno_)
	{
		return m_ref->skipDocCandidate( docno_);
	}

	virtual Index skipPos( const Index& firstpos)
	{
		return m_ref->skipPos( firstpos);
	}

	virtual const char* featureid() const
	{
		return m_ref->featureid();
	}

	virtual Index documentFrequency() const
	{
		return m_ref->documentFrequency();
	}

	virtual unsigned int frequency()
	{
		return m_ref->frequency();
	}

	virtual Index docno() const
	{
		return m_ref->docno();
	}

	virtual Index posno() const
	{
		return m_ref->posno();
	}

private:
	PostingIteratorInterface* m_ref;
};

}//namespace
#endif

