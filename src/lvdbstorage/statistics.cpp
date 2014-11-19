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
#include "statistics.hpp"
#include <boost/atomic.hpp>

using namespace strus;

#ifdef _STRUS_ENABLE_STATISTICS
static boost::atomic<Statistics::ValueType> g_counters[ Statistics::NofTypes];

void Statistics::increment( Type idx)
{
	g_counters[ idx].fetch_add( 1, boost::memory_order_relaxed);
}

Statistics::ValueType Statistics::value( Type idx)
{
	return g_counters[ idx].load( boost::memory_order_relaxed);
}
#endif






