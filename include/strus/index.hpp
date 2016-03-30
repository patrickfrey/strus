/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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

