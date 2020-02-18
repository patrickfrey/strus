/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Helper functions shared by iterators

#ifndef _STRUS_ITERATOR_HELPERS_HPP_INCLUDED
#define _STRUS_ITERATOR_HELPERS_HPP_INCLUDED
#include "strus/storage/index.hpp"
#include "strus/reference.hpp"
#include "strus/postingIteratorInterface.hpp"
#include <vector>
#include <string>

namespace strus
{

void encodeInteger( std::string& buf, int val);

typedef Reference<PostingIteratorInterface> PostingIteratorReference;

std::vector<PostingIteratorReference>
	orderByDocumentFrequency(
		std::vector<PostingIteratorReference>::const_iterator ai,
		const std::vector<PostingIteratorReference>::const_iterator& ae);

Index minDocumentFrequency( const std::vector<PostingIteratorReference>& ar);
Index maxDocumentFrequency( const std::vector<PostingIteratorReference>& ar);


}//namespace
#endif

