/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
/// \brief Numeric types used for local and global indices
/// \file "index.hpp"
#ifndef _STRUS_INDEX_HPP_INCLUDED
#define _STRUS_INDEX_HPP_INCLUDED

#ifdef _MSC_VER
#pragma warning(disable:4290)
#include <BaseTsd.h>
namespace strus {
	///\typedef Index
	///\brief Number type generally used for locally counted indices
	typedef INT32 Index;
	///\typedef GlobalCounter
	///\brief Number type generally used for indices globally shared between different instances of strus
	typedef INT64 GlobalCounter;
}//namespace
#else
#include <stdint.h>
namespace strus {
	///\typedef Index
	///\brief Number type generally used for locally counted indices
	typedef int32_t Index;
	///\typedef GlobalCounter
	///\brief Number type generally used for indices globally shared between different instances of strus
	typedef int64_t GlobalCounter;
}//namespace
#endif
#endif

