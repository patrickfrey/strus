/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Pseudo random number generator for mutations
#ifndef _STRUS_VECTOR_SPACE_MODEL_RANDOM_HPP_INCLUDED
#define _STRUS_VECTOR_SPACE_MODEL_RANDOM_HPP_INCLUDED
#include <vector>

namespace strus {

class Random
{
public:
	Random();
	unsigned int get( unsigned int min_, unsigned int max_);

private:
	unsigned int m_value;
	unsigned int m_incr;
};

}//namespace
#endif

