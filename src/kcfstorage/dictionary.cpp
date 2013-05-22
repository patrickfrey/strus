#include "dictionary.hpp"
#include "podvector.hpp"
#include <cstring>
#include <limits>
#include <boost/cstdint.hpp>
#include <boost/functional/hash.hpp>
#include <boost/shared_ptr.hpp>

using namespace strus;
static boost::hash<std::string> g_hashfunc;

struct Block
{
	Index next;
	Index idx;
	unsigned short keysize;
	char key[1];

	Block()
	{
		std::memset( this, 0, sizeof(*this));
	}
	Block( const Block& o)
	{
		std::memcpy( this, &o, sizeof(*this));
	}
	std::size_t size() const
	{
		return (std::size_t)keysize;
	}
};


struct Dictionary::Impl
{
	Impl( const std::string& type_, const std::string& name_, const std::string& path_)
		:m_ovlfile( filepath( path_, name_, type_ + "ovl"))
		,m_idxvector( path_, name_, type_ + "idx")
		,m_invvector( path_, name_, type_ + "inv"){}

	~Impl()
	{
		close();
	}

	static void create( const std::string& type_, const std::string& name_, const std::string& path_, std::size_t size_)
	{
		Impl impl( type_, name_, path_);
		impl.create( size_);
	}

	void create( std::size_t size_)
	{
		if (size_ < 2) throw std::runtime_error( "invalid size for dictionary");
		m_hashsize = 2;
		while (m_hashsize < size_ && m_hashsize) m_hashsize <<= 2;
		if (!m_hashsize) throw std::bad_alloc();

		m_ovlfile.create();
		m_idxvector.create();
		m_invvector.create();

		Block nullblock;
		m_ovlfile.awrite( &nullblock, sizeof(nullblock));
		m_idxvector.fill( m_hashsize, 0);
	}

	void open( bool writemode_)
	{
		m_ovlfile.open( writemode_);
		m_idxvector.open( writemode_);
		m_invvector.open( writemode_);
		m_hashsize = m_idxvector.size();
	}

	void close()
	{
		if (m_hashsize)
		{
			m_ovlfile.close();
			m_idxvector.close();
			m_invvector.close();
		}
	}

	struct BlockRefMem
	{
		boost::shared_ptr<void> ptr;
		struct
		{
			Block blk;
			char ar[1024];
		} localdef;
	};

	static Block* getBlockRef( BlockRefMem& mem, std::size_t blksize)
	{
		Block* rt;
		if (sizeof( mem.localdef.ar) < blksize)
		{
			mem.ptr = boost::shared_ptr<void>( std::calloc( 1, blksize + sizeof(Block)), std::free);
			rt = (Block*)mem.ptr.get();
		}
		else
		{
			rt = &mem.localdef.blk;
		}
		return rt;
	}

	Index newkeystring( const std::string& key, const Index& idx)
	{
		if (key.size() >= std::numeric_limits<unsigned short>::max())
		{
			throw std::runtime_error( "key size too big (max key size 65535)");
		}
		BlockRefMem mem;
		Block* blkptr = getBlockRef( mem, key.size());

		blkptr->keysize = (unsigned short)key.size();
		std::memcpy( blkptr->key, key.c_str(), blkptr->keysize);
		blkptr->key[ blkptr->keysize] = '\0';
		blkptr->next = 0;
		blkptr->idx = idx;

		return m_ovlfile.awrite( blkptr, sizeof(Block)+blkptr->keysize);
	}

	Index appendkeystring( const std::string& key, Index ovlidx, const Index& idx)
	{
		Block blk;
		for (;;)
		{
			m_ovlfile.pread( ovlidx, &blk, sizeof(Block));
			if (blk.next == 0)
			{
				blk.next = newkeystring( key, idx);
				m_ovlfile.pwrite( ovlidx, &blk, sizeof(Block));
				return blk.next;
			}
			else if (ovlidx >= blk.next)
			{
				throw std::runtime_error( "corrupt index");
			}
			ovlidx = blk.next;
		}
	}

	Index findkeystring( const std::string& key, Index ovlidx) const
	{
		if (key.size() >= std::numeric_limits<unsigned short>::max())
		{
			return 0;
		}
		BlockRefMem mem;
		Block* blkref = getBlockRef( mem, key.size());

		blkref->keysize = (unsigned short)key.size();
		std::memcpy( blkref->key, key.c_str(), blkref->keysize);

		for (;;)
		{
			m_ovlfile.pread( ovlidx, blkref, key.size());
			if (blkref->size() == key.size() && std::strcmp( blkref->key, key.c_str()) == 0)
			{
				return blkref->idx;
			}
			if (blkref->next != 0)
			{
				if (ovlidx >= blkref->next)
				{
					throw std::runtime_error( "corrupt index");
				}
				ovlidx = blkref->next;
			}
			else
			{
				return 0;
			}
		}
	}

	std::string getIdentifier( const Index& idx) const
	{
		if (!idx) throw std::runtime_error( "illegal parameter (null index)");
		Index ovlidx = m_invvector.get( idx-1);

		BlockRefMem mem;
		std::size_t blkrefsize = 128;
		Block* blkref = getBlockRef( mem, blkrefsize);
		m_ovlfile.pread( ovlidx, blkref, blkrefsize);
		if (blkref->size() > blkrefsize)
		{
			blkref = getBlockRef( mem, blkref->size());
			m_ovlfile.pread( ovlidx, blkref, blkref->size());
		}
		return std::string( blkref->key, blkref->size());
	}

	Index find( const std::string& key) const
	{
		if (!m_hashsize) throw std::runtime_error( "reading from dictionary not opened");
		std::size_t idx = g_hashfunc( key) & (m_hashsize -1);
		Index ovlidx = m_idxvector.get( idx);
		if (ovlidx)
		{
			return findkeystring( key, ovlidx);
		}
		else
		{
			return 0;
		}
	}

	Index insert( const std::string& key)
	{
		if (!m_hashsize) throw std::runtime_error( "writing to dictionary not opened");
		std::size_t idx = g_hashfunc( key) & (m_hashsize -1);
		Index ovlidx = m_idxvector.get( idx);
		if (ovlidx)
		{
			Index keyno = m_invvector.push_back( 0);
			ovlidx = appendkeystring( key, ovlidx, keyno + 1);
			m_invvector.set( keyno, ovlidx);
			return keyno + 1;
		}
		else
		{
			Index keyno = m_invvector.push_back( 0);
			ovlidx = newkeystring( key, keyno + 1);
			m_invvector.set( keyno, ovlidx);
			m_idxvector.set( idx, ovlidx);
			return keyno + 1;
		}
	}

private:
	std::size_t m_hashsize;
	File m_ovlfile;
	PodVector<Index> m_idxvector;
	PodVector<Index> m_invvector;
};



void Dictionary::create( const std::string& type_, const std::string& name_, const std::string& path_, std::size_t size_)
{
	Impl impl( type_, name_, path_);
	impl.create( size_);
}

Dictionary::Dictionary( const std::string& type_, const std::string& name_, const std::string& path_)
{
	m_impl = new Impl( type_, name_, path_);
}

Dictionary::~Dictionary()
{
	delete m_impl;
}

Index Dictionary::insert( const std::string& key)
{
	return m_impl->insert( key);
}

Index Dictionary::find( const std::string& key) const
{
	return m_impl->find( key);
}

void Dictionary::create( std::size_t size_)
{
	m_impl->create( size_);
}

void Dictionary::open( bool writemode_)
{
	m_impl->open( writemode_);
}

void Dictionary::close()
{
	m_impl->close();
}

std::string Dictionary::getIdentifier( const Index& idx) const
{
	return m_impl->getIdentifier( idx);
}


