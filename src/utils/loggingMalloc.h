/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_LOGGING_MALLOC_HPP_INCLUDED
#define _STRUS_LOGGING_MALLOC_HPP_INCLUDED
#define _GNU_SOURCE
#include <stdlib.h>

/* This include is just o describe the overloaded functions. Use the library with LD_PRELOAD */

extern void* malloc( size_t size);
extern void* realloc( void* ptr, size_t size);
extern void* calloc( size_t nmemb, size_t size);
extern void* memalign( size_t alignment, size_t size);
extern void free(void *p);

#endif

