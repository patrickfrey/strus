/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Similarity hash structure
#include "simHash.hpp"
#include "private/bitOperations.hpp"
#include "private/internationalization.hpp"
#include <string>
#include <iostream>
#include <sstream>
#include <limits>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace strus;

SimHash::SimHash( std::size_t size_, bool initval)
	:m_ar(),m_size(size_)
{
	if (m_size == 0) return;

	uint64_t elem = (initval)?std::numeric_limits<uint64_t>::max():0;
	std::size_t ii = 0, nn = m_size / NofElementBits;
	for (; ii < nn; ++ii)
	{
		m_ar.push_back( elem);
	}
	if (m_size > nn * NofElementBits)
	{
		elem <<= m_size - (nn * NofElementBits);
		m_ar.push_back( elem);
	}
}

SimHash::SimHash( const std::vector<bool>& bv)
	:m_ar(),m_size(bv.size())
{
	if (bv.empty()) return;
	uint64_t elem = 0;
	std::vector<bool>::const_iterator ai = bv.begin(), ae = bv.end();
	unsigned int aidx = 0;
	for (; ai != ae; ++ai,++aidx)
	{
		if (aidx == (int)NofElementBits)
		{
			m_ar.push_back( elem);
			elem = 0;
			aidx = 0;
		}
		else
		{
			elem <<= 1;
		}
		elem |= *ai ? 1:0;
	}
	elem <<= (NofElementBits - aidx);
	m_ar.push_back( elem);
}

bool SimHash::operator[]( std::size_t idx) const
{
	std::size_t aridx = idx / NofElementBits;
	std::size_t arofs = idx % NofElementBits;
	if (aridx >= m_ar.size()) return false;
	uint64_t mask = 1;
	mask <<= (NofElementBits-1 - arofs);
	return (m_ar[ aridx] & mask) != 0;
}

void SimHash::set( std::size_t idx, bool value)
{
	std::size_t aridx = idx / NofElementBits;
	std::size_t arofs = idx % NofElementBits;
	if (aridx >= m_ar.size())
	{
		throw strus::runtime_error(_TXT("array bound write in %s"), "SimHash");
	}
	uint64_t mask = 1;
	mask <<= (NofElementBits-1 - arofs);
	if (value)
	{
		m_ar[ aridx] |= mask;
	}
	else
	{
		m_ar[ aridx] &= ~mask;
	}
}

std::vector<std::size_t> SimHash::indices( bool what) const
{
	std::vector<std::size_t> rt;
	std::vector<uint64_t>::const_iterator ai = m_ar.begin(), ae = m_ar.end();
	std::size_t aridx = 0;
	for (; ai != ae; ++ai,++aridx)
	{
		std::size_t arofs = 0;
		uint64_t mask = (uint64_t)1 << (NofElementBits-1);
		for (; arofs < NofElementBits; ++arofs,mask>>=1)
		{
			if (((mask & *ai) != 0) == what)
			{
				rt.push_back( aridx * NofElementBits + arofs);
			}
		}
	}
	return rt;
}

unsigned int SimHash::count() const
{
	unsigned int rt = 0;
	std::vector<uint64_t>::const_iterator ai = m_ar.begin(), ae = m_ar.end();
	for (; ai != ae; ++ai)
	{
		rt += strus::BitOperations::bitCount( *ai);
	}
	return rt;
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

bool SimHash::near( const SimHash& o, unsigned int dist) const
{
	unsigned int cnt = 0;
	std::vector<uint64_t>::const_iterator ai = m_ar.begin(), ae = m_ar.end();
	std::vector<uint64_t>::const_iterator oi = o.m_ar.begin(), oe = o.m_ar.end();
	for (; oi != oe && ai != ae; ++oi,++ai)
	{
		cnt += strus::BitOperations::bitCount( *ai ^ *oi);
		if (cnt > dist) return false;
	}
	for (; oi != oe; ++oi)
	{
		cnt += strus::BitOperations::bitCount( *oi);
		if (cnt > dist) return false;
	}
	for (; ai != ae; ++ai)
	{
		cnt += strus::BitOperations::bitCount( *ai);
		if (cnt > dist) return false;
	}
	return true;
}

std::string SimHash::tostring() const
{
	std::ostringstream rt;
	std::vector<uint64_t>::const_iterator ai = m_ar.begin(), ae = m_ar.end();
	for (unsigned int aidx=0; ai != ae; ++ai,++aidx)
	{
		if (aidx) rt << '|';
		uint64_t mi = 1;
		unsigned int cnt = m_size - (aidx * NofElementBits);
		mi <<= (NofElementBits-1);
		for (; mi && cnt; mi >>= 1, --cnt)
		{
			rt << ((*ai & mi)?'1':'0');
		}
	}
	return rt.str();
}

void SimHash::printSerialization( std::string& out, const std::vector<SimHash>& ar)
{
	std::size_t maxsize = 0;
	std::vector<SimHash>::const_iterator vi = ar.begin(), ve = ar.end();
	for (; vi != ve; ++vi)
	{
		if (vi->size() > maxsize)
		{
			maxsize = vi->size();
		}
	}
	std::size_t maxnofelem = maxsize/NofElementBits;
	if (ar.size() > std::numeric_limits<uint32_t>::max()) throw strus::runtime_error(_TXT("sim hash vector too big to serialize"));
	if (maxnofelem > std::numeric_limits<uint32_t>::max()) throw strus::runtime_error(_TXT("sim hash vector elements too big to serialize"));

	uint32_t nw;
	nw = htonl( (uint32_t)maxnofelem);	out.append( (const char*)&nw, sizeof(nw));
	nw = htonl( (uint32_t)ar.size());	out.append( (const char*)&nw, sizeof(nw));
	for (vi = ar.begin(); vi != ve; ++vi)
	{
		std::vector<uint64_t>::const_iterator ai = vi->m_ar.begin(), ae = vi->m_ar.end();
		std::size_t wi = 0, we = maxnofelem;
		for (; ai != ae; ++ai,++wi)
		{
			nw = htonl( *ai >> 32);		out.append( (const char*)&nw, sizeof(nw));
			nw = htonl( *ai & 0xFFffFFffU);	out.append( (const char*)&nw, sizeof(nw));
		}
		for (; wi != we; ++wi)
		{
			nw = 0;
			out.append( (const char*)&nw, sizeof(nw));
			out.append( (const char*)&nw, sizeof(nw));
		}
	}
}

std::vector<SimHash> SimHash::createFromSerialization( const std::string& in, std::size_t& itr)
{
	std::vector<SimHash> rt;
	uint32_t const* nw = (const uint32_t*)(void*)(in.c_str() + itr);
	std::size_t maxnofelem = ntohl( *nw++);
	std::size_t vi=0,ve = ntohl( *nw++);
	for (; vi < ve; ++vi)
	{
		SimHash elem;
		std::size_t ai=0,ae=maxnofelem;
		for (; ai != ae; ++ai)
		{
			uint64_t val = ntohl( *nw++);
			val = (val << 32) | ntohl( *nw++);
			elem.m_ar.push_back( val);
		}
		elem.m_size = maxnofelem * NofElementBits;
		rt.push_back( elem);
	}
	return rt;
}

uint64_t hash64Bitshuffle( uint64_t a)
{
	a = (a+0x7ed55d1617ad327a) + (a<<31);
	a = (a^0xc761c23c384321a7) ^ (a>>19);
	a = (a+0x165667b171b497a3) + (a<<5);
	a = (a+0xd3a2646c61a5cd01) ^ (a<<9);
	a = (a+0xfd7046c529aa46c8) + (a<<41);
	a = (a^0xb55a4f091a99cf51) ^ (a>>17);
	a = (a+0x19fa430a826cd104) + (a<<7);
	a = (a^0xc78123985cfa1097) ^ (a>>27);
	a = (a+0x37af76271ff18537) ^ (a<<12);
	a = (a+0xc16752fa0917283a) + (a<<21);
	return a;
}

SimHash SimHash::randomHash( std::size_t size_, unsigned int seed)
{
	SimHash rt;
	std::size_t ai=0,ae = (size_ + NofElementBits - 1) / NofElementBits;
	rt.m_size = size_;
	rt.m_ar.reserve( ae);
	enum {KnuthConst=2654435761};
	for (; ai != ae; ++ai)
	{
		rt.m_ar.push_back( hash64Bitshuffle( (seed + ai) * KnuthConst));
	}
	return rt;
}



