#include "blockcache.hpp"
#include <cstdlib>
#include <boost/functional/hash.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/taus88.hpp>

using namespace strus;

BlockCache::BlockCache( std::size_t nofblocks_, std::size_t blocksize_)
	:m_ar(0)
	,m_arsize(nofblocks_)
	,m_hashtable(0)
	,m_hashtablesize(nofblocks_*4)
	,m_timestamp(0)
	,m_blocksize(blocksize_)
{
	if (m_arsize == 0 || (m_arsize & (m_arsize -1)) != 0) throw std::runtime_error( "block cache size must be configured as power of 2");
	m_ar = (BlockRef*)std::calloc( m_arsize, sizeof( BlockRef));
	if (!m_ar) goto BAD_ALLOC;
	m_hashtable = (unsigned int*)std::calloc( m_hashtablesize, sizeof(*m_hashtable));
	if (!m_hashtable) goto BAD_ALLOC;

	for (std::size_t ii=0; ii<m_arsize; ++ii)
	{
		m_ar[ii].data = strus::malloc_aligned( m_blocksize, strus::cachelinesize());
		if (!m_ar[ii].data) goto BAD_ALLOC;
	}
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

BlockMetaData* BlockCache::blockMetaData( void* block) const
{
	return (BlockMetaData*)((char*)(block) + m_blocksize);
}

void* BlockCache::allocBlock( unsigned int id)
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
	BlockMetaData* md = blockMetaData( rt);
	if (md->refcnt.fetch_sub( 1) == 1)
	{
		std::free( block);
	}
}

static boost::taus88 g_rndgen;

unsigned int BlockCache::findLRU() const
{
	unsigned int xx = g_rndgen();
	unsigned int aa[5];
	aa[0] = boost::hash( xx + m_timestamp + 0) & (m_arsize-1);
	aa[1] = boost::hash( xx + m_timestamp + 1) & (m_arsize-1);
	aa[2] = boost::hash( xx + m_timestamp + 2) & (m_arsize-1);
	aa[3] = boost::hash( xx + m_timestamp + 3) & (m_arsize-1);
	aa[4] = boost::hash( xx + m_timestamp + 4) & (m_arsize-1);
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

void BlockCache::insertBlock( unsigned int id, void* data)
{
	unsigned int hashval = boost::hash( id) & (m_hashtablesize-1);
AGAIN:
	unsigned int lru_idx = findLRU();
	void* prevdata = m_ar[ lru_idx].data;
	if (!m_ar[ lru_idx].data.compare_exchange_strong( olddata, data))
	{
		goto AGAIN;
	}
	freeBlock( prevdata);
}

void* BlockCache::getBlock( unsigned int id)
{
	unsigned int blockadr = boost::hash( id) & (m_hashtablesize-1);
}




