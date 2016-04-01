/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef STRUS_TEST_LOGGING_MALLOC
#include "loggingMalloc.h"
#define malloc_IMPL malloc
#define calloc_IMPL calloc
#define free_IMPL free
#define realloc_IMPL realloc
#define memalign_IMPL memalign
#endif
#include <dlfcn.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#ifdef __FreeBSD__
#include <malloc_np.h>
#endif
#ifdef __linux__
#include <malloc.h>
#endif
#ifdef __APPLE__
#include <malloc/malloc.h>
#endif
#include <stdlib.h>
#include <libgen.h>
#include <execinfo.h>
#include <unistd.h>
#include "strus/private/snprintf.h"

#undef STRUS_LOWLEVEL_DEBUG

#ifdef STRUS_TEST_LOGGING_MALLOC
#define DLL_PUBLIC static
#else
#if __GNUC__ >= 4
  #define DLL_PUBLIC __attribute__ ((visibility ("default")))
#else
  #define DLL_PUBLIC
#endif
#endif

/// Apple libc spells malloc_usable_size() as malloc_size().
#ifdef __APPLE__
  #define malloc_usable_size(x) malloc_size(x)
#endif

/// brief Forward declaration:
static void print_stacktrace();

static int g_malloc_counter = 0;
static int g_malloc_nofblks = 0;
static int g_malloc_memsize = 0;
static int g_interval_lo = 0;
static int g_interval_hi = 0x7FFFFFFFL;
static int g_interval_cnt = 1000;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static char g_logfile[128];
static int g_logfile_opened = 0;
static int g_callersel = 1;
static char g_callerlib[128];
static char g_callerfunc[128];

#define FRAME_SIGNATURE   0x3141593
#define INITIALIZED_TRUE  0x1234567
#define INITIALIZED_FALSE 0x7654321
static volatile unsigned int g_module_initialized = INITIALIZED_FALSE;

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

static char g_buf_calloc_for_dlsym[64];

static void* g_calloc_for_dlsym( size_t nmemb, size_t size)
{
	static int called = 0;

	if (called || nmemb * size > sizeof(g_buf_calloc_for_dlsym))
	{
		fprintf( stderr, "unexpected calloc(%u,%u) in dlsym\n", (unsigned int)nmemb, (unsigned int)size);
		print_stacktrace();
		return 0;
	}
	memset( g_buf_calloc_for_dlsym, 0, sizeof(g_buf_calloc_for_dlsym));
	++called;
	return g_buf_calloc_for_dlsym;
}

static void* (*g_libc_malloc)( size_t) = 0;
static void* (*g_libc_calloc)( size_t,size_t) = &g_calloc_for_dlsym;
static void* (*g_libc_realloc)( void*,size_t) = 0;
static void* (*g_libc_memalign)( size_t,size_t) = 0;
static void  (*g_libc_free)( void*) = 0;

static void copy_string( char* buf, size_t bufsize, const char* str)
{
	size_t len = strlen(str);
	if (len >= bufsize)
	{
		len = bufsize-1;
	}
	memcpy( buf, str, len);
	buf[ len] = '\0';
}

static void init_module()
{
#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "ENTER init_module\n");
#endif
	/* This function gets a little bit complicated because of
		"ISO C forbids conversion of object pointer to function pointer type" */
	void* calloc_ptr = dlsym( RTLD_NEXT, "calloc");
#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "DONE first dlsym\n");
#endif
	Calloc_s* calloc_ref = (Calloc_s*)&calloc_ptr;
	g_libc_calloc = calloc_ref->func;

#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "GOT a calloc function\n");
#endif
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
	pthread_mutex_init( &g_mutex, NULL);

#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "GOT all memory access functions\n");
#endif
	g_logfile[0] = '\0';
	const char* nm = getenv( "STRUS_MALLOC_LOGFILE");
	if (nm && nm[0])
	{
		if (nm[0] != '/')
		{
			char dirbuf[128];
			char* dir = getcwd( dirbuf, sizeof(dirbuf));
			strus_snprintf( g_logfile, sizeof(g_logfile), "%s/%s", dir, nm);
		}
		else
		{
			copy_string( g_logfile, sizeof(g_logfile), nm);
		}
		fprintf( stderr, "malloc logging to file '%s'\n", g_logfile);
	}
	nm = getenv( "STRUS_MALLOC_INTERVAL");
	if (nm && nm[0])
	{
		int lo,hi;
		int nofitems = sscanf( nm, "%d:%d:%d", &g_interval_cnt, &lo, &hi);
		if (nofitems < 1)
		{
			fprintf( stderr, "syntax error in value of the environment variable STRUS_MALLOC_INTERVAL. expected 3 integers separated by ':'\n");
		}
		else
		{
			if (nofitems >= 2) g_interval_lo = lo;
			if (nofitems >= 3) g_interval_hi = hi;

			if (g_interval_lo > g_interval_hi)
			{
				/* swap lower and upper bound if they are in wrong order: */
				int tmp = g_interval_lo;
				g_interval_lo = g_interval_hi;
				g_interval_hi = tmp;
			}
			if (g_interval_cnt < 2)
			{
				fprintf( stderr, "error in value of the environment variable STRUS_MALLOC_INTERVAL: first value must be bigger than 1\n");
				g_interval_cnt = 2;
			}
			fprintf( stderr, "malloc logging snapshot every %d malloc from size %d to size %d\n", g_interval_cnt, g_interval_lo, g_interval_hi);
		}
	}
	g_callerlib[0] = '\0';
	nm = getenv( "STRUS_MALLOC_CALLERLIB");
	if (nm && nm[0])
	{
		copy_string( g_callerlib, sizeof(g_callerlib), nm);
		fprintf( stderr, "restrict logging to calls from library '%s'\n", g_callerlib);
	}
	g_callerfunc[0] = '\0';
	nm = getenv( "STRUS_MALLOC_CALLERFUNC");
	if (nm && nm[0])
	{
		copy_string( g_callerfunc, sizeof(g_callerfunc), nm);
		fprintf( stderr, "restrict logging to calls from function '%s'\n", g_callerfunc);
	}
	nm = getenv( "STRUS_MALLOC_CALLER");
	if (nm && nm[0])
	{
		if (!g_callerfunc[0] && !g_callerlib[0])
		{
			fprintf( stderr, "environment variable STRUS_MALLOC_CALLER is ignored when not specifying any of STRUS_MALLOC_CALLERFUNC and STRUS_MALLOC_CALLERLIB\n");
		}
		if ((nm[0]|32) == 'y' || nm[0] == '1' || (nm[0]|32) == 't')
		{
			g_callersel = 1;
		}
		else if ((nm[0]|32) == 'n' || nm[0] == '0' || (nm[0]|32) == 'f')
		{
			g_callersel = 0;
			fprintf( stderr, "inverting set of malloc calls logged\n");
		}
		else
		{
			fprintf( stderr, "error in value of the environment variable STRUS_MALLOC_CALLER: must be Y/YES/T/TRUE/1 or N/NO/F/0\n");
		}
	}
	fprintf( stderr, "logging malloc initialized\n");
}

static void check_init()
{
	if (g_module_initialized != INITIALIZED_TRUE)
	{
		g_module_initialized = INITIALIZED_TRUE;
		init_module();
	}
}

#define MAX_STACKTRACE 128
static int match_dlname( const char* dlname, const char* name)
{
	return (dlname && NULL!=strstr( dlname, name));
}

struct stack_frame {
	struct stack_frame* next;
	void* return_address;
};

size_t stacktrace( void** trace, size_t tracesize)
{
	struct stack_frame* current;
	struct stack_frame* previous;
	void* frame_ptr = __builtin_frame_address(0);
	size_t rt = 0;

	/* We skip the call to this function, it makes no sense to record it.  */
	current = ((struct stack_frame*) frame_ptr);
	while (rt < tracesize && current)
	{
		if (current->return_address == 0 || current->next == 0) break;
		trace[ rt++] = current->return_address;
		previous = current;
		current = current->next;
		if (current > previous || current + 10000 < previous) break;
	}
	return rt;
}

static int find_sym( const char* libname, const char* symname)
{
	size_t si,se;
	void* trace[ MAX_STACKTRACE];

	se = stacktrace( trace, MAX_STACKTRACE);
	for (si=0; si<se; ++si)
	{
		Dl_info info;
		if (dladdr( trace[si], &info))
		{
			if (libname && symname)
			{
				if (match_dlname( info.dli_fname, libname)
				&&  match_dlname( info.dli_sname, symname))
				{
					break;
				}
			}
			else if (libname)
			{
				if (match_dlname( info.dli_fname, libname)) break;
			}
			else if (symname)
			{
				if (match_dlname( info.dli_sname, symname)) break;
			}
		}
	}
	return (si < se);
}

static void print_stacktrace()
{
	size_t si,se;
	void* trace[ MAX_STACKTRACE];
	se = stacktrace( trace, MAX_STACKTRACE);

	fprintf( stderr, "STK:\n");
	fprintf( stderr, "size %u:\n", (unsigned int)se);
	for (si=0; si<se; ++si)
	{
		Dl_info info;
		if (dladdr( trace[si], &info))
		{
			fprintf( stderr, "\t%s\t%s\n", info.dli_fname?basename((char *)info.dli_fname):"(unknown)", info.dli_sname?info.dli_sname:"(unknown)");
		}
	}
}

static inline int caller_match()
{
	if (!g_callerlib[0] && !g_callerfunc[0]) return 1;
	if (find_sym( g_callerlib[0]?g_callerlib:0, g_callerfunc[0]?g_callerfunc:0))
	{
		return g_callersel;
	}
	else
	{
		return !g_callersel;
	}
}

static void stat_malloc_call( size_t size)
{
	pthread_mutex_lock( &g_mutex);
	if ((size_t)g_interval_lo <= size && (size_t)g_interval_hi >= size)
	{
		g_malloc_memsize += size;
		g_malloc_nofblks += 1;
	}
	g_malloc_counter += 1;
	pthread_mutex_unlock( &g_mutex);
}

static void stat_free_call( size_t size)
{
	pthread_mutex_lock( &g_mutex);
	if ((size_t)g_interval_lo <= size && (size_t)g_interval_hi >= size)
	{
		g_malloc_memsize -= size;
		g_malloc_nofblks -= 1;
	}
	pthread_mutex_unlock( &g_mutex);
}

static void log_statistics()
{
	const char* logfile_attrib = 0;
	int do_log = 0;
	struct
	{
		int memsize;
		int nofblks;
	}
	info;
	pthread_mutex_lock( &g_mutex);
	if (g_malloc_counter >= g_interval_cnt)
	{
		g_malloc_counter = 0;
		do_log = 1;
		if (g_logfile_opened)
		{
			logfile_attrib = "a+";
		}
		else
		{
			g_logfile_opened = 1;
			logfile_attrib = "w";
		}
		info.memsize = g_malloc_memsize;
		info.nofblks = g_malloc_nofblks;
	}
	pthread_mutex_unlock( &g_mutex);

	if (do_log)
	{
		FILE* logfile = stderr;
		if (g_logfile[0])
		{
			logfile = fopen( g_logfile, logfile_attrib);
			if (!logfile)
			{
				fprintf( stderr, "cannot open malloc log file '%s' (errno %u)", g_logfile, errno);
				g_logfile[0] = '\0';
		}
		}
		if (logfile)
		{
			fprintf( logfile, "%d %d\n", info.nofblks, info.memsize);
		}
		if (logfile != stderr)
		{
			fclose( logfile);
		}
	}
}



typedef struct mem_frame
{
	int signature;
	int caller_match;
} mem_frame;

DLL_PUBLIC void* malloc_IMPL( size_t size)
{
	mem_frame* frame;

#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "ENTER malloc( %u)\n", (unsigned int)size);
#endif
	check_init();
#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "CALL glibc malloc( %u)\n", (unsigned int)size);
#endif
	frame = (mem_frame*)g_libc_malloc( sizeof(mem_frame) + size);
#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "CALLED glibc malloc %x\n", (unsigned int)(uintptr_t)frame);
#endif
	if (!frame) return 0;
	frame->signature = FRAME_SIGNATURE;
	frame->caller_match = caller_match();
	void* rt = (void*)(frame+1);
	size_t rt_size = malloc_usable_size( frame) - sizeof(mem_frame);

	if (frame->caller_match)
	{
		stat_malloc_call( rt_size);
	}
	log_statistics();

#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "CALLED malloc( %u)%s RETURNS %x\n", (unsigned int)size, (frame->caller_match?" logged":""), (unsigned int)(uintptr_t)rt);
#endif
	return rt;
}

DLL_PUBLIC void* realloc_IMPL( void* ptr, size_t size)
{
	mem_frame* old_frame;
	size_t old_size;
	int old_caller_match;
	mem_frame* new_frame;
	void* rt;
	size_t rt_size;

	if (ptr == NULL) return malloc_IMPL( size);

#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "ENTER realloc( %x, %u)\n", (unsigned int)(uintptr_t)ptr, (unsigned int)size);
#endif
	check_init();
	old_frame = ((mem_frame*)ptr) - 1;
	old_size = malloc_usable_size( old_frame) - sizeof(mem_frame);
	old_caller_match = old_frame->caller_match;
	if (old_caller_match > 1 || old_frame->signature != FRAME_SIGNATURE)
	{
		fprintf( stderr, "invalid realloc( %x, %u, %x, %d)\n", (unsigned int)(uintptr_t)ptr, (unsigned int)size, old_frame->signature, old_caller_match);
		print_stacktrace();
	}
#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "CALLING glibc realloc( %u %x)\n", (unsigned int)old_size, (unsigned int)(uintptr_t)old_frame);
#endif
	new_frame = (mem_frame*)g_libc_realloc( (void*)old_frame, sizeof(mem_frame) + size);
#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "CALLED glibc realloc %x\n", (unsigned int)(uintptr_t)new_frame);
#endif
	if (!new_frame) return 0;
	new_frame->signature = FRAME_SIGNATURE;
	new_frame->caller_match = caller_match();
	rt = (void*)(new_frame+1);
	rt_size = malloc_usable_size( new_frame) - sizeof(mem_frame);

	if (old_caller_match)
	{
		stat_free_call( old_size);
	}
	if (new_frame->caller_match)
	{
		stat_malloc_call( rt_size);
	}

#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "CALLED realloc( %x, %u)%s RETURNS %x\n", (unsigned int)(uintptr_t)ptr, (unsigned int)size, (new_frame->caller_match?" logged":""), (unsigned int)(uintptr_t)rt);
#endif
	return rt;
}

DLL_PUBLIC void* calloc_IMPL( size_t nmemb, size_t size)
{
	mem_frame* frame;
	void* rt;
	size_t rt_size;

#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "ENTER calloc( %u,%u)\n", (unsigned int)nmemb, (unsigned int)size);
#endif
	check_init();
#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "CALLING glibc calloc( %u,%u)\n", (unsigned int)nmemb, (unsigned int)size);
#endif
	frame = (mem_frame*)g_libc_calloc( sizeof(mem_frame) + nmemb * size, 1);
#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "CALLED glibc calloc %x\n", (unsigned int)(uintptr_t)frame);
#endif
	if (!frame) return 0;
	frame->signature = FRAME_SIGNATURE;
	if ((void*)frame == &g_buf_calloc_for_dlsym) return (void*)(frame+1);

	frame->caller_match = caller_match();
	rt = (void*)(frame+1);
	rt_size = malloc_usable_size( frame) - sizeof(mem_frame);

	if (frame->caller_match)
	{
		stat_malloc_call( rt_size);
	}
	log_statistics();

#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "CALLED calloc( %u, %u)%s RETURNS %x\n", (unsigned int)nmemb, (unsigned int)size, (frame->caller_match?" logged":""), (unsigned int)(uintptr_t)rt);
#endif
	return rt;
}

DLL_PUBLIC void* memalign_IMPL( size_t alignment, size_t size)
{
	return malloc_IMPL( size);
	/* ... we do not care about alignment in this library and hope nothing will crash :-( */
}

DLL_PUBLIC void free_IMPL( void* ptr)
{
	mem_frame* frame;
	size_t size;

#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "ENTER free(%x)\n", (unsigned int)(uintptr_t)ptr);
#endif
	if (ptr == 0) return;

	check_init();
	frame = (mem_frame*)ptr - 1;
	if ((void*)frame == &g_buf_calloc_for_dlsym) return;

	if (frame->signature != FRAME_SIGNATURE)
	{
		fprintf( stderr, "invalid free( %x) bad signature: %x\n", (unsigned int)(uintptr_t)ptr, frame->signature);
		print_stacktrace();
	}
#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "EVAL usable size\n");
#endif
	size = malloc_usable_size( (void*)frame) - sizeof(mem_frame);
#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "GOT usable size %u\n", (unsigned int)size);
#endif
	if (frame->caller_match > 1)
	{
		fprintf( stderr, "invalid free( %x) caller match %d\n", (unsigned int)(uintptr_t)ptr, (int)frame->caller_match);
		print_stacktrace();
	}
	int caller_match = frame->caller_match;
	if (caller_match)
	{
		stat_free_call( size);
	}
	log_statistics();

#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "CALLING glibc free( %x)\n", (unsigned int)(uintptr_t)frame);
#endif
	g_libc_free( (void*)frame);

#ifdef STRUS_LOWLEVEL_DEBUG
	fprintf( stderr, "CALLED free()%s\n", caller_match?" logged":"");
#endif
}

