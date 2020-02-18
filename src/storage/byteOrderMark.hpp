/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_BYTE_ORDER_MARK_HPP_INCLUDED
#define _STRUS_STORAGE_BYTE_ORDER_MARK_HPP_INCLUDED
#include "strus/storage/index.hpp"
#include <cstring>

namespace strus {

class ByteOrderMark
{
public:
	ByteOrderMark()
	{
		std::memset( this, 0, sizeof(*this));
		for (int ii=0; ii<4; ++ii) data.ar[ii] = ii;
	}

	void set( const Index& val)
	{
		data.val = val;
	}

	Index value() const
	{
		return data.val;
	}

	const char* endianess() const
	{
		if (data.val == 0x01020304)
		{
			return "big endian";
		}
		if (data.val == 0x04030201)
		{
			return "little endian";
		}
		return "endianess unknown (corrupt data)";
	}

private:
	union
	{
		Index val;
		char ar[4];
	} data;
};

} //namespace
#endif

