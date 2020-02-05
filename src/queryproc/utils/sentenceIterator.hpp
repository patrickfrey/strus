/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_QUERYPROC_SENTENCE_ITERATOR_HPP_INCLUDED
#define _STRUS_QUERYPROC_SENTENCE_ITERATOR_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/base/string_format.hpp"
#include "strus/postingIteratorInterface.hpp"
#include <utility>

namespace strus {

class SentenceIterator
{
public:
	SentenceIterator( PostingIteratorInterface* eos_iter_, strus::Index docno, const strus::IndexRange& field_, strus::Index maxSentenceSize_);

	strus::IndexRange skipPos( strus::Index pos);

private:
	PostingIteratorInterface* m_eos_iter;
	strus::IndexRange m_field;
	strus::Index m_maxSentenceSize;
};

}//namespace
#endif

