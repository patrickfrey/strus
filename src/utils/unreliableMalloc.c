/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "unreliableMalloc.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#if __GNUC__ >= 4
  #define DLL_PUBLIC __attribute__ ((visibility ("default")))
  #define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
#else
  #define DLL_PUBLIC
  #define DLL_LOCAL
#endif

#undef STRUS_LOWLEVEL_DEBUG
#define MALLOC_FAILURE_COUNTER_LIMIT 100000
static unsigned int g_malloc_counter = 0;
static unsigned int g_malloc_limit = MALLOC_FAILURE_COUNTER_LIMIT;

#define INITIALIZED_TRUE  0x1234567
#define INITIALIZED_FALSE 0x7654321
static unsigned int g_module_initialized = INITIALIZED_FALSE;

typedef void* (*Malloc_f)( size_t);
typedef void* (*Calloc_f)( size_t,size_t);
typedef void* (*Realloc_f)( void*,size_t);
typedef void* (*Memalign_f)( size_t,size_t);
typedef void  (*Free_f)( void*);

typedef struct Malloc_s {Malloc_f func;} Malloc_s;
typedef struct Calloc_s {Calloc_f func;} Calloc_s;
typedef struct Realloc_s {Realloc_f func;} Realloc_s;
typedef struct Memalign_s {Memalign_f func;} Memalign_s;
typedef struct Free_s {Free_f func;} Free_s;

static void* g_calloc_for_dlsym( size_t nmemb, size_t size)
{
	static int called = 0;
	static char buf[ 32];

	if (called || nmemb * size > sizeof(buf))
	{
		fprintf( stderr, "unexpected calloc in dlsym\n");
		exit( 1);
	}
	memset( buf, 0, sizeof(buf));
	++called;
	return buf;
}

static void* (*g_libc_malloc)( size_t) = 0;
static void* (*g_libc_calloc)( size_t,size_t) = &g_calloc_for_dlsym;
static void* (*g_libc_realloc)( void*,size_t) = 0;
static void* (*g_libc_memalign)( size_t,size_t) = 0;
static void  (*g_libc_free)( void*) = 0;

static void init_module()
{
	const char* nm = getenv( "STRUS_MALLOC_FAILURE_RATE");
	if (nm)
	{
		sscanf( nm, "%u", &g_malloc_limit);
		fprintf( stderr, "malloc failure occurring every %u mallocs\n", g_malloc_limit);
	}

	/* This function gets a little bit complicated because of
		"ISO C forbids conversion of object pointer to function pointer type" */
	void* calloc_ptr = dlsym( RTLD_NEXT, "calloc");
	Calloc_s* calloc_ref = (Calloc_s*)&calloc_ptr;
	g_libc_calloc = calloc_ref->func;

	void* malloc_ptr = dlsym( RTLD_NEXT, "malloc");
	Malloc_s* malloc_ref = (Malloc_s*)&malloc_ptr;
	g_libc_malloc = malloc_ref->func;

	void* realloc_ptr = dlsym( RTLD_NEXT, "realloc");
	Realloc_s* realloc_ref = (Realloc_s*)&realloc_ptr;
	g_libc_realloc = realloc_ref->func;

	void* memalign_ptr = dlsym( RTLD_NEXT, "memalign");
	Memalign_s* memalign_ref = (Memalign_s*)&memalign_ptr;
	g_libc_memalign = memalign_ref->func;

	void* free_ptr = dlsym( RTLD_NEXT, "free");
	Free_s* free_ref = (Free_s*)&free_ptr;
	g_libc_free = free_ref->func;

	if (!calloc_ptr||!malloc_ptr||!realloc_ptr||!memalign_ptr||!free_ptr)
	{
		fprintf( stderr, "error loading malloc function with 'dlsym'\n");
		exit( 1);
	}

	fprintf( stderr, "warning: unreliable malloc initialized\n");
}

static int failure()
{
	if (g_module_initialized != INITIALIZED_TRUE)
	{
		g_module_initialized = INITIALIZED_TRUE;
		init_module();
	}
	if (++g_malloc_counter >= g_malloc_limit)
	{
		fprintf( stderr, "error: unreliable malloc returns malloc failure\n");
		g_malloc_counter = 0;
		return 1;
	}
	else
	{
		return 0;
	}
}

DLL_PUBLIC void* malloc( size_t size)
{
	void* rt = failure()?0:g_libc_malloc( size);
#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "CALL malloc( %u) RETURNS %x\n", (unsigned int)size, (unsigned int)(uintptr_t)rt);
#endif
	return rt;
}

DLL_PUBLIC void* realloc( void* ptr, size_t size)
{
	void* rt = failure()?0:g_libc_realloc( ptr, size);
#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "CALL realloc( %x, %u) RETURNS %x\n", (unsigned int)(uintptr_t)ptr, (unsigned int)size, (unsigned int)(uintptr_t)rt);
#endif
	return rt;
}

DLL_PUBLIC void* calloc( size_t nmemb, size_t size)
{
	void* rt = failure()?0:g_libc_calloc( nmemb, size);
#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "CALL calloc( %u, %u) RETURNS %x\n", (unsigned int)nmemb, (unsigned int)size, (unsigned int)(uintptr_t)rt);
#endif
	return rt;
}

DLL_PUBLIC void* memalign( size_t alignment, size_t size)
{
	void* rt = failure()?0:g_libc_memalign( alignment, size);
#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "CALL memalign( %u, %u) RETURNS %x\n", (unsigned int)alignment, (unsigned int)size, (unsigned int)(uintptr_t)rt);
#endif
	return rt;
}

DLL_PUBLIC void free( void* ptr)
{
	g_libc_free( ptr);
#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "CALL free( %x)\n", (unsigned int)(uintptr_t)ptr);
#endif
}

