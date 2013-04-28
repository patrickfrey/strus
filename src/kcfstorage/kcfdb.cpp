#include "kcfdb.hpp"
#include <kcdbext.h>
#include <cstdio>
#include <algorithm>
#include <cstdlib>
#include <boost/lexical_cast.hpp>

#ifdef _MSC_VER
#define FILE_PATH_DELIMITER '\\'
#else
#define FILE_PATH_DELIMITER '/'
#endif
#define SET_DEFAULT_DBPATH(PATH)\
static const char* default_dbpath = "" #PATH;
SET_DEFAULT_DBPATH(STRUS_DEFAULT_DBPATH)

using namespace strus;

static std::string filename( const std::string& path, const std::string& name, const std::string& ext)
{
	std::string rt;
	rt.append( path);
	rt.push_back( FILE_PATH_DELIMITER);
	rt.append( name);
	rt.push_back( '.');
	rt.append( ext);
	std::replace( rt.begin(), rt.end(), '/', FILE_PATH_DELIMITER);
	return rt;
}

struct StorageVariables
{
	DocNumber docCounter;
};

struct StorageDB::Impl
{
	Impl( const std::string& name_, const std::string& path_);
	~Impl() {close();}
	void close();
	void open();
	static void create( const std::string& name_, const std::string& path_);

	std::string name;
	std::string path;
	bool dociddb_open;
	kyotocabinet::IndexDB dociddb;
	bool termdb_open;
	kyotocabinet::IndexDB termdb;
	enum {SmallBlockSize=128};
	FILE* smallblkh;
	enum {BigBlockSize=4096};
	FILE* indexblkh;

	typedef unsigned int BlockNumber;
	std::vector<BlockNumber> blockmap;
	std::map<DocNumber,bool> deletedDocMap;
	typedef std::map<std::string,unsigned short> TypeMap;
	TypeMap typemap;
	StorageVariables variables;
};

void StorageDB::clearTransaction()
{
	m_transaction.docmap.clear();
	m_transaction.docCounter = m_impl->variables.docCounter;
	m_transaction.errormsg.clear();
	m_transaction.errorno = 0;
}

struct Block
{
	void* ar;
	std::size_t size;

	Block()
		:ar(0),size(0){}
	~Block()
	{
		if (ar) std::free( ar);
	}
};

static std::runtime_error FileError( const std::string& msg)
{
	return std::runtime_error( msg + "(system error code " + boost::lexical_cast<std::string>((int)errno) + ")");
}

static FILE* openBlockFile( const std::string& filename)
{
	FILE* fh = fopen( filename.c_str() , "r+");
	if (!fh)
	{
		throw FileError( std::string( "failed to open file '") + filename + "' for reading");
	}
	return fh;
}

static void createFile( const std::string& filename, int initSize=0)
{
	FILE* fh = fopen( filename.c_str() , "w");
	if (!fh)
	{
		throw FileError( std::string( "failed to create file '") + filename + "' for reading");
	}
	if (initSize)
	{
		char* ptr = (char*)std::calloc( initSize, 1);
		if (!ptr) throw std::bad_alloc();
		fwrite( ptr, 1, initSize, fh);
		std::free( ptr);
	}
	fclose( fh);
}

static void writeFile( const std::string& filename, void* blk, std::size_t blksize)
{
	FILE* fh = fopen( filename.c_str() , "w");
	if (!fh)
	{
		throw FileError( std::string( "failed to open file '") + filename + "' for writing");
	}
	std::size_t writelen = fwrite( (char*)blk, 1, blksize, fh);
	if (writelen < blksize)
	{
		fclose( fh);
		throw FileError( std::string( "failed to write to file '") + filename + "'");
	}
	fclose( fh);
}

static void readFile( const std::string& filename, Block& blk)
{
	FILE* fh = fopen( filename.c_str() , "r");
	if (!fh)
	{
		throw FileError( std::string( "failed to open file '") + filename + "' for reading");
	}
	fseek( fh, 0L, SEEK_END);
	blk.size = ftell( fh);
	fseek( fh, 0L, SEEK_SET);

	if (blk.ar) std::free( blk.ar);
	if (blk.size)
	{
		blk.ar = std::malloc( blk.size);
		if (!blk.ar)
		{
			fclose( fh);
			throw std::bad_alloc();
		}
	}
	else
	{
		blk.ar = 0;
	}
	std::size_t readlen;

	readlen = fread( (char*)blk.ar, 1, blk.size, fh);
	if (readlen < blk.size)
	{
		fclose( fh);
		throw FileError( std::string( "failed to read from file '") + filename + "'");
	}
	fclose( fh);
}

StorageDB::Impl::Impl( const std::string& name_, const std::string& path_)
	:name(name_),path(path_),dociddb_open(false),termdb_open(false),smallblkh(0),indexblkh(0)
{}

void StorageDB::Impl::close()
{
	writeFile( filename( path, name, "variables"), &variables, sizeof(variables));

	if (dociddb_open) dociddb.close();
	if (termdb_open) termdb.close();
	if (smallblkh) fclose( smallblkh);
	if (indexblkh) fclose( indexblkh);
}

void StorageDB::Impl::create( const std::string& name_, const std::string& path_)
{
	createFile( filename( path_, name_, "deleted"));
	createFile( filename( path_, name_, "blocknum"));
	createFile( filename( path_, name_, "small"));
	createFile( filename( path_, name_, "index"));
	createFile( filename( path_, name_, "type"));
	createFile( filename( path_, name_, "variables"), sizeof(variables));

	kyotocabinet::IndexDB dociddb;
	kyotocabinet::IndexDB termdb;

	if (!dociddb.open( filename( path_, name_, "dociddb"), kyotocabinet::BasicDB::OCREATE))
	{
		throw std::runtime_error( std::string("failed to create docid table '") + filename( path_, name_, "dociddb") + "'");
	}
	if (!termdb.open( filename( path_, name_, "termdb"), kyotocabinet::BasicDB::OCREATE))
	{
		throw std::runtime_error( std::string("failed to create term table '") + filename( path_, name_, "termdb") + "'");
	}
}

void StorageDB::Impl::open()
{
	try
	{
		Block blk;
		// read the map of files marked as deleted:
		readFile( filename( path, name, "deleted"), blk);
		if (blk.size % sizeof(DocNumber) != 0) throw std::runtime_error( std::string("corrupted file '") + path + "' for deleted document numbers");
		DocNumber* delar = (DocNumber*) blk.ar;
		std::size_t di=0,delsize = blk.size / sizeof(DocNumber);
		for (; di<delsize; ++di) deletedDocMap[ delar[ di]] = true;

		// read the map of term numbers to block numbers:
		readFile( filename( path, name, "blocknum"), blk);
		if (blk.size % sizeof(BlockNumber) != 0) throw std::runtime_error( std::string("corrupted file '") + path + "' for block numbers");
		BlockNumber* blnar = (BlockNumber*) blk.ar;
		std::size_t bi=0,blnsize = blk.size / sizeof(DocNumber);
		for (; bi<blnsize; ++bi) blockmap.push_back( blnar[bi]);

		// read the map of term type strings to type numbers:
		readFile( filename( path, name, "type"), blk);
		char* tp = (char*) blk.ar;
		std::size_t ti=0;
		for (; ti<blk.size; ++ti)
		{
			typemap[ std::string(tp+ti)] = typemap.size()+1;
			for (; ti<blk.size && tp[ ti]; ++ti);
		}

		// read the storage variables:
		readFile( filename( path, name, "variables"), blk);
		if (blk.size != sizeof(variables))
		{
			throw std::runtime_error( "corrupt storage variables file");
		}
		std::memcpy( &variables, blk.ar, sizeof(variables));

		smallblkh = openBlockFile( filename( path, name, "small"));
		indexblkh = openBlockFile( filename( path, name, "index"));

		if (!dociddb.open( filename( path, name, "dociddb"), kyotocabinet::BasicDB::OWRITER))
		{
			throw std::runtime_error( std::string("failed to open docid table '") + filename( path, name, "dociddb") + "'");
		}
		if (!termdb.open( filename( path, name, "termdb"), kyotocabinet::BasicDB::OWRITER))
		{
			throw std::runtime_error( std::string("failed to open term table '") + filename( path, name, "termdb") + "'");
		}
	}
	catch (std::runtime_error& e)
	{
		close();
		throw e;
	}
	catch (std::bad_alloc& e)
	{
		close();
		throw e;
	}
}

StorageDB::StorageDB( const std::string& name, const std::string& path)
	:m_impl( new Impl( name, path))
{
	m_impl->open();
}

StorageDB::StorageDB( const std::string& name)
	:m_impl( new Impl( name, default_dbpath))
{
	m_impl->open();
}

void StorageDB::create( const std::string& name, const std::string& path)
{
	Impl::create( name, path);
}

void StorageDB::create( const std::string& name)
{
	Impl::create( name, default_dbpath);
}

StorageDB::~StorageDB()
{
	delete m_impl;
}

std::string StorageDB::getTermKey( const std::string& type, const std::string& value) const
{
	std::string key;
	Impl::TypeMap::const_iterator ti = m_impl->typemap.find( type);
	if (ti == m_impl->typemap.end()) return 0;
	unsigned short typenum = ti->second;
	if (typenum < 127)
	{
		key.push_back( typenum);
	}
	else
	{
		key.push_back( (typenum >> 8) + 0x80);
		key.push_back( typenum & 0x7F);
	}
	key.append( value);
	return key;
}

TermNumber StorageDB::findTermNumber( const std::string& key) const
{
	std::size_t ptrsize = 0;
	char* ptr = m_impl->termdb.get( key.c_str(), key.size(), &ptrsize);
	if (ptrsize != sizeof( TermNumber))
	{
		throw std::runtime_error( "corrupt index termdb");
	}
	TermNumber rt;
	std::memcpy( &rt, ptr, ptrsize);
	delete [] ptr;
	return rt;
}

TermNumber StorageDB::findTermNumber( const std::string& type, const std::string& value) const
{
	return findTermNumber( getTermKey( type, value));
}

DocNumber StorageDB::findDocumentNumber( const std::string& docid) const
{
	std::size_t ptrsize = 0;
	char* ptr = m_impl->dociddb.get( docid.c_str(), docid.size(), &ptrsize);
	if (ptrsize != sizeof( DocNumber))
	{
		throw std::runtime_error( "corrupt index dociddb");
	}
	DocNumber rt;
	std::memcpy( &rt, ptr, ptrsize);
	delete [] ptr;
	return rt;
}

TermNumber StorageDB::insertTermNumber( const std::string& type, const std::string& value)
{
	std::string key( getTermKey( type, value));
	TermNumber rt = findTermNumber( key);
	if (rt) return rt;
	m_impl->blockmap.push_back( 0);
	rt = m_impl->blockmap.size();
	if (!m_impl->termdb.set( key.c_str(), key.size(), (char*)&rt, sizeof(rt)))
	{
		throw std::runtime_error( std::string("failed to create term '") + value + "':" + type);
	}
	return rt;
}

DocNumber StorageDB::insertDocumentNumber( const std::string& docid)
{
	DocNumber rt = ++m_transaction.docCounter;
	m_transaction.docmap[ docid] = rt;
	return rt;
}

void StorageDB::begin()
{
	clearTransaction();
}

bool StorageDB::commit()
{
	std::map<std::string, DocNumber>::const_iterator di = m_transaction.docmap.begin(), de = m_transaction.docmap.end();
	for (; di != de; ++di)
	{
		if (!m_impl->dociddb.set( di->first.c_str(), di->first.size(), (char*)&di->second, sizeof(di->second)))
		{
			kyotocabinet::BasicDB::Error kcferr = m_impl->dociddb.error();
			m_transaction.errormsg = std::string("failed to create document id for '") + di->first + "' (" + kcferr.message() + ")";
			m_transaction.errorno = KCF_ERRORBASE + (int)kcferr.code();
			return false;
		}
	}
	return true;
}

void StorageDB::rollback()
{
	clearTransaction();
}

std::string StorageDB::lastError()
{
	return m_transaction.errormsg;
}

int StorageDB::lastErrno()
{
	return m_transaction.errorno;
}

