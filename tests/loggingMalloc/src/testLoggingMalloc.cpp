/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#define STRUS_TEST_LOGGING_MALLOC
#include "loggingMalloc.c"
#include <stdexcept>

uint32_t uint32_hash( uint32_t a)
{
	a += ~(a << 15);
	a ^=  (a >> 10);
	a +=  (a << 3);
	a ^=  (a >> 6);
	a += ~(a << 11);
	a ^=  (a >> 16);
	return a;
}

class Random
{
public:
	explicit Random( unsigned int seed)
	{
		time_t nowtime;
		struct tm* now;

		::time( &nowtime);
		now = ::localtime( &nowtime);

		m_value = uint32_hash( ((now->tm_year+1+seed)
					* (now->tm_mon+100+seed)
					* (now->tm_mday+1+seed)));
	}

	unsigned int get( unsigned int min_, unsigned int max_)
	{
		if (min_ >= max_)
		{
			throw std::runtime_error("illegal range passed to pseudo random number generator");
		}
		m_value = uint32_hash( m_value + 1);
		unsigned int iv = max_ - min_;
		if (iv)
		{
			return (m_value % iv) + min_;
		}
		else
		{
			return min_;
		}
	}

private:
	unsigned int m_value;
};


int main( int argc, const char** argv)
{
	try
	{
		unsigned int seed = 0;
		if (argc == 4)
		{
			seed = (unsigned int)atoi( argv[3]);
		}
		Random g_random( seed);
		if (argc < 3)
		{
			throw std::runtime_error( "missing argument <nof allocs> <mem size>");
		}
		if (argv[1][0] < '0' || argv[1][0] > '9')
		{
			throw std::runtime_error( "illegal first argument: non negative number expected");
		}
		if (argv[2][0] < '0' || argv[2][0] > '9')
		{
			throw std::runtime_error( "illegal second argument: non negative number expected");
		}
		unsigned int nofAllocs = (unsigned int)atoi( argv[1]);
		unsigned int memSize = (unsigned int)atoi( argv[2]);
		unsigned int incMemSize = memSize/10;
		memSize -= incMemSize;
		if (memSize < nofAllocs)
		{
			throw std::runtime_error( "second argument (memory to use) expected to be bigger than first (number of alloc operations)");
		}
		void** ar = (void**)calloc( nofAllocs, sizeof(void**));
		unsigned int* sizear = (unsigned int*)calloc( nofAllocs, sizeof(unsigned int));
		unsigned int ai = 0;

		for (ai=0; ai < nofAllocs; ++ai)
		{
			sizear[ ai] = (memSize / nofAllocs);
		}
		unsigned int restmem = memSize - (memSize / nofAllocs);
		for (ai=0; ai < 100; ++ai)
		{
			sizear[ g_random.get( 0, nofAllocs)] += restmem/100; 
		}
		for (ai=0; ai < nofAllocs; ++ai)
		{
			unsigned int from_i = g_random.get( 0, nofAllocs), to_i = g_random.get( 0, nofAllocs);
			unsigned int mm = sizear[ from_i] / 2;
			if (sizear[ from_i] > mm)
			{
				sizear[ from_i] -= mm;
				sizear[ to_i] += mm;
			}
		}
		for (ai=0; ai < nofAllocs*4; ++ai)
		{
			unsigned int from_i = g_random.get( 0, nofAllocs), to_i = g_random.get( 0, nofAllocs);
			unsigned int mm = g_random.get( 1, sizear[ from_i] / 3 + 2);
			if (sizear[ from_i] > mm)
			{
				sizear[ from_i] -= mm;
				sizear[ to_i] += mm;
			}
		}

		const unsigned int dist[10] = {0,1,0,0,2,1,0,1,2,0};
		unsigned int lastAllocIdx = 0;
		for (ai=0; ai < nofAllocs; ++ai)
		{
			AGAIN:
			{
				unsigned int what = dist[ g_random.get(0,10)];
				switch (what)
				{
					case 0:
					{
						unsigned int mm = sizear[ ai];
						for (; lastAllocIdx < ai; ++lastAllocIdx)
						{
							mm += sizear[ lastAllocIdx];
						}
						unsigned int malloc_type = g_random.get(0,4);
						switch (malloc_type)
						{
							case 0:
								ar[ ai] = malloc_IMPL( mm);
								break;
							case 1:
								ar[ ai] = realloc_IMPL( 0, mm);
								break;
							case 2:
								ar[ ai] = memalign_IMPL( 8, mm);
								break;
							case 3:
							{
								unsigned int elemsize = 1;
								unsigned int blksize = mm;
								while (g_random.get(0,2) == 0)
								{
									elemsize *= 2;
									blksize /= 2;
								}
								if (blksize == 0)
								{
									elemsize = 1;
									blksize = mm;
								}
								ar[ ai] = calloc_IMPL( blksize, elemsize);
								break;
							}
						}
						break;
					}
					case 1:
					{
						if (ai == 0) goto AGAIN;
						unsigned int freeBlockIdx = g_random.get(0,ai)+1;
						for (;freeBlockIdx > 0; --freeBlockIdx)
						{
							if (ar[ freeBlockIdx-1])
							{
								free_IMPL( ar[ freeBlockIdx-1]);
								ar[ freeBlockIdx-1] = 0;
								break;
							}
						}
						if (freeBlockIdx == 0) goto AGAIN;
						break;
					}
					case 2:
					{
						if (ai == 0 || incMemSize == 0) goto AGAIN;
						unsigned int freeBlockIdx = g_random.get(0,ai)+1;
						for (;freeBlockIdx > 0; --freeBlockIdx)
						{
							if (ar[ freeBlockIdx-1])
							{
								unsigned int incmem = g_random.get(1,100);
								if (incmem > incMemSize)
								{
									incmem = incMemSize;
								}
								incMemSize -= incmem;
								ar[ freeBlockIdx-1] = realloc_IMPL( ar[ freeBlockIdx-1], sizear[ freeBlockIdx] + incmem);
								sizear[ freeBlockIdx] += incmem;
								break;
							}
						}
						if (freeBlockIdx == 0) goto AGAIN;
						break;
					}
					default:
						throw std::logic_error("this cannot happen");
				}
			}
		}
		for (ai=0; ai < nofAllocs; ++ai)
		{
			if (ar[ ai])
			{
				free_IMPL( ar[ ai]);
				ar[ ai] = 0;
			}
		}
		free( ar);
		free( sizear);
		return 0;
	}
	catch (const std::exception& err)
	{
		fprintf( stderr, "exceptipon: %s\n", err.what());
	}
	return -1;
}


