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
#include "strus/base/string_format.hpp"
#include "strus/postingIteratorInterface.hpp"
#include <utility>

namespace strus {

class StructureIterator
{
public:
	StructureIterator()
		:m_valid_structar(0),m_structarSize(0),m_windowsize(0),m_cur(0,0){}
	StructureIterator( Index windowsize_, PostingIteratorInterface** valid_structar_, std::size_t structarSize_)
		:m_valid_structar(valid_structar_),m_structarSize(structarSize_),m_windowsize(windowsize_),m_cur(0,0){}

	void init( Index windowsize_, PostingIteratorInterface** valid_structar_, std::size_t structarSize_);

	std::pair<Index,Index> skipPos( strus::Index posno);

private:
	PostingIteratorInterface** m_valid_structar;
	std::size_t m_structarSize;
	Index m_windowsize;
	std::pair<Index,Index> m_cur;
};

}//namespace
#endif

