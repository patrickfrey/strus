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
#include "loggingMalloc.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <malloc.h>

#if __GNUC__ >= 4
  #define DLL_PUBLIC __attribute__ ((visibility ("default")))
  #define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
#else
  #define DLL_PUBLIC
  #define DLL_LOCAL
#endif

#undef STRUS_LOWLEVEL_DEBUG
static unsigned int g_malloc_counter = 0;
static unsigned int g_malloc_nofblks = 0;
static unsigned int g_malloc_memsize = 0;
static unsigned int g_interval_lo = 0;
static unsigned int g_interval_hi = 0xFFFFFFFFUL;
static unsigned int g_interval_cnt = 1000;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static char g_logfile[128];

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
	pthread_mutex_init( &g_mutex, NULL);

	const char* nm = getenv( "STRUS_MALLOC_LOGFILE");
	if (nm)
	{
		size_t nmlen = strlen(nm);
		if (nmlen >= sizeof(g_logfile))
		{
			nmlen = sizeof(g_logfile)-1;
		}
		memcpy( g_logfile, nm, nmlen);
		g_logfile[ nmlen] = '\0';
		fprintf( stderr, "malloc logging to file '%s'\n", g_logfile);
	}
	else
	{
		g_logfile[0] = '\0';
	}
	nm = getenv( "STRUS_MALLOC_INTERVAL");
	if (nm)
	{
		sscanf( nm, "%u:%u:%u", &g_interval_cnt, &g_interval_lo, &g_interval_hi);
		if (g_interval_lo > g_interval_hi)
		{
			/* swap lower and upper bound if they are in wrong order: */
			unsigned int tmp = g_interval_lo;
			g_interval_lo = g_interval_hi;
			g_interval_hi = tmp;
		}
		fprintf( stderr, "malloc logging every %u malloc from size %u to size %u\n", g_interval_cnt, g_interval_lo, g_interval_hi);
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

	fprintf( stderr, "logging malloc initialized\n");
}

static void log_malloc_call( size_t size)
{
	if (g_module_initialized != INITIALIZED_TRUE)
	{
		g_module_initialized = INITIALIZED_TRUE;
		init_module();
	}
	pthread_mutex_lock( &g_mutex);
	if (g_interval_lo <= size && g_interval_hi >= size)
	{
		g_malloc_memsize += size;
		g_malloc_nofblks += 1;
	}
	g_malloc_counter += 1;

	if (g_malloc_counter >= g_interval_cnt)
	{
		FILE* logfile = stderr;
		if (g_logfile[0])
		{
			logfile = fopen( "a+", g_logfile);
			if (!logfile)
			{
				fprintf( stderr, "cannot open malloc log file '%s' (errno %u)", g_logfile, errno);
				logfile = stderr;
			}
		}
		fprintf( logfile, "%u %u\n", g_malloc_nofblks, g_malloc_memsize);
		if (logfile != stderr)
		{
			fclose( logfile);
		}
		g_malloc_counter = 0;
	}
	pthread_mutex_unlock( &g_mutex);
}

static void log_free_call( size_t size)
{
	if (g_module_initialized != INITIALIZED_TRUE)
	{
		g_module_initialized = INITIALIZED_TRUE;
		init_module();
	}
	pthread_mutex_lock( &g_mutex);
	if (g_interval_lo <= size && g_interval_hi >= size)
	{
		g_malloc_memsize -= size;
		g_malloc_nofblks -= 1;
	}
	pthread_mutex_unlock( &g_mutex);
}

DLL_PUBLIC void* malloc( size_t size)
{
	void* rt = g_libc_malloc( size);
	log_malloc_call( malloc_usable_size( rt));
#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "CALL malloc( %u) RETURNS %x\n", (unsigned int)size, (unsigned int)(uintptr_t)rt);
#endif
	return rt;
}

DLL_PUBLIC void* realloc( void* ptr, size_t size)
{
	size_t oldsize = malloc_usable_size( ptr);
	void* rt = g_libc_realloc( ptr, size);
	size_t newsize = malloc_usable_size( rt);
	if (newsize > oldsize)
	{
		log_malloc_call( newsize - oldsize);
	}
	else if (newsize < oldsize)
	{
		log_free_call( oldsize - newsize);
	}
#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "CALL realloc( %x, %u) RETURNS %x\n", (unsigned int)(uintptr_t)ptr, (unsigned int)size, (unsigned int)(uintptr_t)rt);
#endif
	return rt;
}

DLL_PUBLIC void* calloc( size_t nmemb, size_t size)
{
	void* rt = g_libc_calloc( nmemb, size);
	log_malloc_call( malloc_usable_size( rt));

#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "CALL calloc( %u, %u) RETURNS %x\n", (unsigned int)nmemb, (unsigned int)size, (unsigned int)(uintptr_t)rt);
#endif
	return rt;
}

DLL_PUBLIC void* memalign( size_t alignment, size_t size)
{
	void* rt = g_libc_memalign( alignment, size);
	log_malloc_call( malloc_usable_size( rt));

#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "CALL memalign( %u, %u) RETURNS %x\n", (unsigned int)alignment, (unsigned int)size, (unsigned int)(uintptr_t)rt);
#endif
	return rt;
}

DLL_PUBLIC void free( void* ptr)
{
	size_t size = malloc_usable_size( ptr);
	log_free_call( size);
	g_libc_free( ptr);

#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "CALL free( %x)\n", (unsigned int)(uintptr_t)ptr);
#endif
}

