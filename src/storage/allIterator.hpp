/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_ALL_ITERATOR_HPP_INCLUDED
#define _STRUS_STORAGE_ALL_ITERATOR_HPP_INCLUDED
#include "strus/postingIteratorInterface.hpp"

namespace strus {

/// \brief Iterator representing the complete set of document numbers
class AllIterator
	:public PostingIteratorInterface
{
public:
	explicit AllIterator( Index maxDocno_)
		:m_maxDocno(maxDocno_),m_docno(0){}

	virtual ~AllIterator(){}

	virtual std::vector<const PostingIteratorInterface*>
			subExpressions( bool positive) const
	{
		return std::vector<const PostingIteratorInterface*>();
	}

	virtual const char* featureid() const
	{
		return "*";
	}

	virtual Index skipDoc( const Index& docno_)
	{
		return skipDocImpl( docno_);
	}

	virtual Index skipDocCandidate( const Index& docno_)
	{
		return skipDocImpl( docno_);
	}

	virtual Index skipPos( const Index& posno_)
	{
		return 0;
	}

	virtual unsigned int frequency()
	{
		return 0;
	}

	virtual Index documentFrequency() const
	{
		return m_maxDocno;
	}
	
	virtual Index docno() const
	{
		return m_docno;
	}

	virtual Index posno() const
	{
		return 0;
	}

private:
	Index skipDocImpl( const Index& docno_)
	{
		if (docno_ <= m_maxDocno)
		{
			if (docno_)
			{
				return m_docno = docno_;
			}
			else
			{
				return m_docno = 1;
			}
		}
		return 0;
	}

private:
	Index m_maxDocno;
	Index m_docno;
};

}
#endif
