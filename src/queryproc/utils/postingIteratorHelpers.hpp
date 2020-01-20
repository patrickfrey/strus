/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_QUERYPROC_POSTING_ITERATOR_HELPERS_HPP_INCLUDED
#define _STRUS_QUERYPROC_POSTING_ITERATOR_HELPERS_HPP_INCLUDED
#include "private/internationalization.hpp"
#include "strus/base/string_format.hpp"
#include "strus/postingIteratorInterface.hpp"
#include <utility>

namespace strus {

bool callSkipDoc( strus::Index docno, PostingIteratorInterface** ar, std::size_t arsize, PostingIteratorInterface** valid_ar);

Index callSkipPos( strus::Index start, PostingIteratorInterface** ar, std::size_t size);

std::pair<Index,Index> callSkipPosWithLen( strus::Index start, PostingIteratorInterface** ar, std::size_t size);

}//namespace
#endif

