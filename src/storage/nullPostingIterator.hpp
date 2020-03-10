/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_NULL_POSTING_ITERATOR_HPP_INCLUDED
#define _STRUS_STORAGE_NULL_POSTING_ITERATOR_HPP_INCLUDED
#include "strus/postingIteratorInterface.hpp"
#include <string>

namespace strus {

/// \brief Iterator representing an empty set
class NullPostingIterator
	:public PostingIteratorInterface
{
public:
	NullPostingIterator( const char* termstr);

	virtual ~NullPostingIterator(){}

	virtual const char* featureid() const
	{
		return m_featureid.c_str();
	}

	virtual Index skipDoc( const Index&)
	{
		return 0;
	}

	virtual Index skipDocCandidate( const Index&)
	{
		return 0;
	}

	virtual Index skipPos( const Index&)
	{
		return 0;
	}

	virtual int frequency()
	{
		return 0;
	}

	virtual GlobalCounter documentFrequency() const
	{
		return 0;
	}
	
	virtual Index docno() const
	{
		return 0;
	}

	virtual Index posno() const
	{
		return 0;
	}

	virtual Index length() const
	{
		return 0;
	}

private:
	std::string m_featureid;
};

}
#endif
