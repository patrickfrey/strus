/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
///\brief Some non standard functions from the C library for platforms that do not support them
#ifndef _STRUS_CSTRING_HPP_INCLUDED
#define _STRUS_CSTRING_HPP_INCLUDED
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

///\brief Function/macro implementing GNU extension memrchr() (glibc >= 2.1.91)
#include <limits.h>
#if defined( __GNU_LIBRARY__ ) || defined( __GLIBC_ )
#if __GLIBC_PREREQ(2, 2)
#define strus_memrchr(s,c,n)	memrchr(s,c,n)
#else
extern const char* strus_memrchr( const char *s, int c, size_t n);
#endif
#else
extern const char* strus_memrchr( const char *s, int c, size_t n);
#endif

#ifdef __cplusplus
}
#endif
#endif
