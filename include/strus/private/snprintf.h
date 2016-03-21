/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
///\brief Simple implementation of snprintf only supporing a small subset of format characters that is guaranteed not to use malloc
#ifndef _STRUS_SNPRINTF_HPP_INCLUDED
#define _STRUS_SNPRINTF_HPP_INCLUDED
#include <stdlib.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void strus_vsnprintf( char* buf, size_t bufsize, const char* format, va_list arg);
extern void strus_snprintf( char* buf, size_t bufsize, const char* format, ...);

#ifdef __cplusplus
}
#endif
#endif
