/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_FLOAT_CONVERSIONS_HPP_INCLUDED
#define _STRUS_STORAGE_FLOAT_CONVERSIONS_HPP_INCLUDED
#include "strus/index.hpp"
#include <string>
#include <cstdlib>

namespace strus
{

typedef uint16_t float16_t;

float16_t floatSingleToHalfPrecision( float in);
float floatHalfToSinglePrecision( float16_t in);

}//namespace
#endif

