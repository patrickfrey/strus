/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_QUERYPROC_STRUCTURE_ITERATOR_HPP_INCLUDED
#define _STRUS_QUERYPROC_STRUCTURE_ITERATOR_HPP_INCLUDED
#include "private/internationalization.hpp"
#include "strus/index.hpp"
#include "strus/base/string_format.hpp"
#include "strus/postingIteratorInterface.hpp"
#include <utility>

namespace strus {

class SentenceIterator
{
public:
	SentenceIterator()
		:m_delimar(0),m_structarSize(0),m_windowsize(0),m_cur(0,0){}
	SentenceIterator( Index windowsize_, PostingIteratorInterface** delimar_, std::size_t structarSize_)
		:m_delimar(delimar_),m_structarSize(structarSize_),m_windowsize(windowsize_),m_cur(0,0){}

	void init( Index windowsize_, PostingIteratorInterface** delimar_, std::size_t structarSize_);

	strus::IndexRange skipPos( strus::Index posno);

private:
	PostingIteratorInterface** m_delimar;
	std::size_t m_structarSize;
	Index m_windowsize;
	strus::IndexRange m_cur;
};

}//namespace
#endif

