/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_QUERYPROC_UTILS_STRUCTURE_SEARCH_HPP_INCLUDED
#define _STRUS_QUERYPROC_UTILS_STRUCTURE_SEARCH_HPP_INCLUDED
/// \brief Functions to collect structure header and content fields
#include "strus/index.hpp"
#include <vector>

namespace strus
{
/// \brief Forward declaration
class StructIteratorInterface;

/// \brief Function to collect structure header and content fields
/// \return true if found, false else
void collectFieldHeaders( std::vector<strus::IndexRange>& res, StructIteratorInterface* structIter, strus::Index structno, const strus::IndexRange& field);


}//namespace
#endif

