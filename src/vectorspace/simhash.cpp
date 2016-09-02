/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Similarity hash structure
#include "simhash.hpp"
#include "private/bitOperations.hpp"
#include <string>
#include <iostream>
#include <sstream>

using namespace strus;

SimHash::SimHash( const std::vector<bool>& bv)
	:m_ar()
{
	std::size_t idx = 0;
	for (; idx < bv.size(); idx+=NofElementBits)
	{
		uint64_t elem = 0;
		std::vector<bool>::const_iterator ai = bv.begin() + idx, ae = bv.end();
		for (unsigned int aidx=0; ai != ae && aidx < NofElementBits; ++ai,++aidx)
		{
			elem <<= 1;
			elem |= *ai ? 1:0;
		}
		m_ar.push_back( elem);
	}
}

unsigned int SimHash::dist( const SimHash& o) const
{
	unsigned int rt = 0;
	std::vector<uint64_t>::const_iterator ai = m_ar.begin(), ae = m_ar.end();
	std::vector<uint64_t>::const_iterator oi = o.m_ar.begin(), oe = o.m_ar.end();
	for (; oi != oe && ai != ae; ++oi,++ai)
	{
		rt += strus::BitOperations::bitCount( *ai ^ *oi);
	}
	for (; oi != oe; ++oi)
	{
		rt += strus::BitOperations::bitCount( *oi);
	}
	for (; ai != ae; ++ai)
	{
		rt += strus::BitOperations::bitCount( *ai);
	}
	return rt;
}

std::string SimHash::tostring() const
{
	std::ostringstream rt;
	std::vector<uint64_t>::const_iterator ai = m_ar.begin(), ae = m_ar.end();
	for (unsigned int aidx=0; ai != ae; ++ai,++aidx)
	{
		if (aidx) rt << '|';
		uint64_t mi = 1;
		mi <<= 63;
		while (mi)
		{
			rt << ((*ai & mi)?'1':'0');
			mi >>= 1;
		}
	}
	return rt.str();
}





