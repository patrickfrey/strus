/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Header of a statistics message
/// \file statisticsHeader.hpp
#ifndef _STRUS_STATISTICS_HEADER_HPP_INCLUDED
#define _STRUS_STATISTICS_HEADER_HPP_INCLUDED
#include <cstring>
#include <stdint.h>

namespace strus
{

struct StatisticsHeader
{
	StatisticsHeader()
	{
		std::memset( this, 0, sizeof(*this));
	}
	bool empty() const			{return nofDocumentsInsertedChange==0;}

	uint32_t nofDocumentsInsertedChange;
};

/*
 * Message format:
 * [StatisticsHeader] [nof bytes last msg (UTF-8)] [nof bytes rest msg (UTF-8)] [rest msg bytes]
 */
}//namespace
#endif


