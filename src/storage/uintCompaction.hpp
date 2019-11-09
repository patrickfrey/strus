/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_UINT_COMPACTION_HPP_INCLUDED
#define _STRUS_STORAGE_UINT_COMPACTION_HPP_INCLUDED
#include "strus/index.hpp"
#include <string>
#include <cstdlib>

namespace strus
{

uint8_t compactUint( unsigned int in);
unsigned int uintFromCompaction( uint8_t in);

}//namespace
#endif

