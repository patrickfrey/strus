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
#include "strus/storage/index.hpp"
#include "strus/storage/structureHeaderField.hpp"
#include <vector>

namespace strus
{
/// \brief Forward declaration
class StructureIteratorInterface;

/// \brief Function to collect structure header and content fields
/// \return true if found, false else
void collectHeaderFields(
		std::vector<StructureHeaderField>& res,
		StructureIteratorInterface* structIter,
		strus::Index structno,
		strus::Index docno, 
		const strus::IndexRange& field);


}//namespace
#endif

