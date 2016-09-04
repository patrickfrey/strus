/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Pseudo random number generator for mutations
#include "random.hpp"
#include "private/internationalization.hpp"
#include "strus/base/stdint.h"
#include <ctime>

using namespace strus;

/// \brief Pseudo random generator 
enum {KnuthIntegerHashFactor=2654435761U};
#undef STRUS_LOWLEVEL_DEBUG

static inline uint32_t uint32_hash( uint32_t a)
{
	a += ~(a << 15);
	a ^=  (a >> 10);
	a +=  (a << 3);
	a ^=  (a >> 6);
	a += ~(a << 11);
	a ^=  (a >> 16);
	return a;
}

Random::Random()
{
	time_t nowtime;
	struct tm* now;

	::time( &nowtime);
	now = ::localtime( &nowtime);

	m_value = uint32_hash( ((now->tm_year+1)
				* (now->tm_mon+100)
				* (now->tm_mday+1)));
	m_incr = m_value * KnuthIntegerHashFactor;
}

unsigned int Random::get( unsigned int min_, unsigned int max_)
{
	if (min_ >= max_)
	{
		throw strus::runtime_error( _TXT("illegal range passed to pseudo random number generator"));
	}
	m_value = uint32_hash( m_value + 1 + m_incr++);
	unsigned int iv = max_ - min_;
	return (m_value % iv) + min_;
}


