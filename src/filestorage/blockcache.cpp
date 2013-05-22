#include "blockcache.hpp"
#include "malloc.hpp"
#include <cstdlib>
#include <boost/functional/hash.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/taus88.hpp>

using namespace strus;

struct BlockCache::BlockMetaData
{
	BlockNumber id;				//< id of this block
	boost::atomic<unsigned int> refcnt;	//< reference counting of this block
};

struct BlockCache::BlockRef
{
	boost::atomic<unsigned int> gate;	//< non blocking mutual exclusion for one writer (insertBlock)/many reader (getBlock)
	unsigned int timestamp;			//< last used counter for LRU replacement of blocks
	void* data;				//< block data cached
};

BlockCache::BlockCache( std::size_t nofblocks_, std::size_t blocksize_)
	:m_ar(0)
	,m_arsize(nofblocks_)
	,m_hashtable(0)
	,m_hashtablesize(nofblocks_*8)
	,m_timestamp(0)
	,m_blocksize(blocksize_)
	,m_arpos(0)
	,m_filled(false)

{
	if (m_arsize == 0 || (m_arsize & (m_arsize -1)) != 0) throw std::runtime_error( "block cache size must be configured as power of 2");
	m_ar = (BlockRef*)std::calloc( m_arsize, sizeof( BlockRef));
	if (!m_ar) goto BAD_ALLOC;
	m_hashtable = (unsigned int*)std::calloc( m_hashtablesize, sizeof(*m_hashtable));
	if (!m_hashtable) goto BAD_ALLOC;
	return;

BAD_ALLOC:
	if (m_hashtable) std::free( m_hashtable);
	if (m_ar)
	{
		for (std::size_t ii=0; ii<m_arsize; ++ii)
		{
			if (m_ar[ii].data) std::free( m_ar[ii].data);
		}
		std::free( m_ar);
	}
	throw std::bad_alloc();
}

BlockCache::~BlockCache()
{
	for (std::size_t ii=0; ii<m_arsize; ++ii)
	{
		std::free( m_ar[ii].data);
	}
}

BlockCache::BlockMetaData* BlockCache::blockMetaData( void* block) const
{
	return (BlockMetaData*)((char*)(block) + m_blocksize);
}

void* BlockCache::allocBlock( BlockNumber id)
{
	void* rt = strus::malloc_aligned( m_blocksize + sizeof(BlockMetaData), strus::cachelinesize());
	if (!rt) throw std::bad_alloc();
	BlockMetaData* md = blockMetaData( rt);
	md->refcnt = 1;
	md->id = id;
	return rt;
}

void BlockCache::freeBlock( void* block) const
{
	BlockMetaData* md = blockMetaData( block);
	if (md->refcnt.fetch_sub( 1) == 1)
	{
		std::free( block);
	}
}

void BlockCache::referenceBlock( void* block) const
{
	BlockMetaData* md = blockMetaData( block);
	md->refcnt.fetch_add( 1);
}

// random number generator for pseudo LRU pick
static boost::taus88 g_rndgen;
// hash functions for blocks
static boost::hash<BlockNumber> g_hashfunc;

static unsigned int hashfunc( BlockNumber val)
{
	static int rndseed = g_rndgen();
	return (unsigned int)g_hashfunc( rndseed + val);
}

unsigned int BlockCache::findLRU() const
{
	unsigned int xx = g_rndgen();
	unsigned int aa[5];
	aa[0] = hashfunc( xx + m_timestamp + 0) & (m_arsize-1);
	aa[1] = hashfunc( xx + m_timestamp + 1) & (m_arsize-1);
	aa[2] = hashfunc( xx + m_timestamp + 2) & (m_arsize-1);
	aa[3] = hashfunc( xx + m_timestamp + 3) & (m_arsize-1);
	aa[4] = hashfunc( xx + m_timestamp + 4) & (m_arsize-1);

	unsigned int min_tm = m_ar[ aa[0]].timestamp - m_timestamp;
	unsigned int lru_idx = aa[0];
	for (int ii=1; ii<5; ++ii)
	{
		unsigned int tt = m_ar[ aa[ ii]].timestamp - m_timestamp;
		if (tt < min_tm)
		{
			min_tm = tt;
			lru_idx = aa[ ii];
		}
	}
	return lru_idx;
}

void BlockCache::insertBlock( void* block)
{
	BlockCache::BlockMetaData* md = BlockCache::blockMetaData( block);
	unsigned int hashval = hashfunc( md->id) & (m_hashtablesize-1);
	unsigned int lru_idx;
	unsigned int gate = 0;
AGAIN:
	if (m_filled)
	{
		do
		{
			lru_idx = findLRU();
		}
		while (!m_ar[ lru_idx].gate.compare_exchange_strong( gate, 1));
		// ... exclusive block reference aquisition for writer excluding all readers and writers
	}
	else
	{
		// the block cache is not yet filled, so we do an ordinary add without LRU replacement
		lru_idx = m_arpos.fetch_add( 1);
		if (lru_idx > m_arsize -1)
		{
			m_arpos.fetch_sub( 1);
			m_filled = true;
			goto AGAIN;
			//... two insertBlock on the last element, one of them has to try it via LRU replacement
		}
	}
	void* prevdata = m_ar[ lru_idx].data;
	m_ar[ lru_idx].data = block;

	m_ar[ lru_idx].gate.fetch_sub( 1);
	m_hashtable[ hashval] = lru_idx;
	if (prevdata)
	{
		///... block is 0 for an ordinary add
		freeBlock( prevdata);
	}
}

void* BlockCache::getBlock( BlockNumber id)
{
	unsigned int hashval = hashfunc( id) & (m_hashtablesize-1);
	unsigned int idx = m_hashtable[ hashval];

	if ((m_ar[ idx].gate.fetch_add( 2) & 1) != 0)
	{
		// a writer is occupying, we return and get the block in the writer context as we would load it from disk
		m_ar[ idx].gate.fetch_sub( 2);
		return 0;
	}
	// ... reader excluding writer and allowing readers

	void* rt = m_ar[ idx].data;
	referenceBlock( rt);
	// ... 'referenceBlock' must be called before releasing the gate because otherwise the block could be freed before beeing referenced

	m_ar[ idx].gate.fetch_sub( 2);

	BlockMetaData* md = blockMetaData( rt);
	if (md->id != id)
	{
		freeBlock( rt);	//... miss: block occupied by another
		return 0;
	}
	return rt;
}




