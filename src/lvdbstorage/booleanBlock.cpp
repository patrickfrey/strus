/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#include "booleanBlock.hpp"
#include <limits>

using namespace strus;

bool BooleanBlock::append( const Index& elemno, const Index&)
{
	switch (m_class)
	{
		case BitField:
		{
			break;
		}
		case RangeField16:
		{
			int16_t range[2];
			if (m_rangeField16.getTopRange( range[0], range[1]))
			{
			}
			else
			{
				Index elemidx = relativeIndexFromElemno( elemno);
				if (elemidx > std::numeric_limits<int16_t>::max())
				{
					return false;
				}
				int16_t range[2];
				m_rangeField16.packRange( range, )
			}
			break;
		}
		case RangeField32:
		{
			break;
		}
	}
}

BooleanBlock BooleanBlock::merge( const BooleanBlock& newblk, const BooleanBlock& oldblk)
{
	
}

private:
BlockImplClass m_class;
BitField m_bitField;
RangeField<signed short> m_rangeField16;
RangeField<int32_t> m_rangeField32;
