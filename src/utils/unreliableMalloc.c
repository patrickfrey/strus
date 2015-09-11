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
#include "unreliableMalloc.h"
#include <dlfcn.h>
#include <stdio.h>
#include <pthread.h>


#define MALLOC_FAILURE_COUNTER_LIMIT 1000
static unsigned int g_malloc_counter = 0;

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

static void* (*g_libc_malloc)( size_t) = 0;
static void* (*g_libc_calloc)( size_t,size_t) = 0;
static void* (*g_libc_realloc)( void*,size_t) = 0;
static void* (*g_libc_memalign)( size_t,size_t) = 0;
static void  (*g_libc_free)( void*) = 0;

static void init_module()
{
	/* This function gets a little bit complicated because of
		"ISO C forbids conversion of object pointer to function pointer type" */
	{
		void* ptr = dlsym( RTLD_NEXT, "malloc");
		Malloc_s* obj = (Malloc_s*)&ptr;
		g_libc_malloc = obj->func;
	}
	{
		void* ptr = dlsym( RTLD_NEXT, "calloc");
		Calloc_s* obj = (Calloc_s*)&ptr;
		g_libc_calloc = obj->func;
	}
	{
		void* ptr = dlsym( RTLD_NEXT, "realloc");
		Realloc_s* obj = (Realloc_s*)&ptr;
		g_libc_realloc = obj->func;
	}
	{
		void* ptr = dlsym( RTLD_NEXT, "memalign");
		Memalign_s* obj = (Memalign_s*)&ptr;
		g_libc_memalign = obj->func;
	}
	{
		void* ptr = dlsym( RTLD_NEXT, "free");
		Free_s* obj = (Free_s*)&ptr;
		g_libc_free = obj->func;
	}
}

static int failure()
{
	if (g_module_initialized != INITIALIZED_TRUE)
	{
		g_module_initialized = INITIALIZED_TRUE;
		init_module();
	}
	if (++g_malloc_counter >= MALLOC_FAILURE_COUNTER_LIMIT)
	{
		fprintf( stderr, "malloc failed\n");
		g_malloc_counter = 0;
		return 1;
	}
	else
	{
		return 0;
	}
}

void* malloc( size_t size)
{
	return failure()?0:g_libc_malloc( size);
}

void* realloc( void* ptr, size_t size)
{
	return failure()?0:g_libc_realloc( ptr, size);
}

void* calloc( size_t nmemb, size_t size)
{
	return failure()?0:g_libc_calloc( nmemb, size);
}

void* memalign( size_t alignment, size_t size)
{
	return failure()?0:g_libc_memalign( alignment, size);
}

void free( void* ptr)
{
	g_libc_free( ptr);
}

