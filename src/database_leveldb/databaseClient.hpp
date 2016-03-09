/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#ifndef _STRUS_DATABASE_CLIENT_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_DATABASE_CLIENT_IMPLEMENTATION_HPP_INCLUDED
#include "strus/databaseClientInterface.hpp"
#include <leveldb/db.h>
#include "private/utils.hpp"

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Shared handle for accessing Level DB
class LevelDbHandle
{
public:
	/// \brief Constructor
	/// \param[in] path_ path of the storage
	/// \param[in] maxOpenFiles_ maximum number of files open (0 for default)
	/// \param[in] cachesize_k_ number of K LRU cache for nodes
	/// \param[in] compression_ wheter to use snappy compression (true) or not
	/// \param[in] writeBufferSize_ size of write buffer per file
	/// \param[in] blockSize_ block size on disk (size of units)
	LevelDbHandle( const std::string& path_,
			unsigned int maxOpenFiles_,
			unsigned int cachesize_k_,
			bool compression_,
			unsigned int writeBufferSize_,
			unsigned int blockSize_);

	/// \brief Destructor
	~LevelDbHandle();

	const std::string& path() const			{return m_path;}
	leveldb::DB* db() const				{return m_db;}
	unsigned int maxOpenFiles() const		{return m_maxOpenFiles;}
	unsigned int cachesize_k() const		{return m_cachesize_k;}
	unsigned int writeBufferSize() const		{return m_writeBufferSize;}
	unsigned int blockSize() const			{return m_blockSize;}
	bool compression() const			{return m_compression;}

private:
	void cleanup();

private:
	std::string m_path;				///< path to level DB storage directory
	leveldb::Options m_dboptions;			///< options for level DB
	leveldb::DB* m_db;				///< levelDB handle
	unsigned int m_maxOpenFiles;			///< maximum number of files to be opened by Level DB
	unsigned int m_cachesize_k;			///< kilobytes of LRU cache to use
	bool m_compression;				///< true if compression enabled
	unsigned int m_writeBufferSize;			///< size of write buffer (default 4M)
	unsigned int m_blockSize;			///< block unit size (default 4K)
};

/// \brief Map of shared Level DB handles
class LevelDbHandleMap
{
public:
	/// \brief Constructor
	LevelDbHandleMap(){}
	/// \brief Destructor
	~LevelDbHandleMap(){}

	/// \brief Create a new handle or return a reference to an instance already in use
	/// \param[in] path_ path of the storage
	/// \param[in] maxOpenFiles_ maximum number of files open (0 for default)
	/// \param[in] cachesize_k_ number of K LRU cache for nodes
	/// \param[in] compression_ wheter to use snappy compression (true) or not
	/// \param[in] writeBufferSize_ size of write buffer per file
	/// \param[in] blockSize_ block size on disk (size of units)
	/// \note the method throws if the configuration parameters are incompatible to an existing instance
	utils::SharedPtr<LevelDbHandle> create(
			const std::string& path_,
			unsigned int maxOpenFiles,
			unsigned int cachesize_k,
			bool compression,
			unsigned int writeBufferSize_,
			unsigned int blockSize_);

	/// \brief Dereference the handle for the database referenced by path and dispose the handle, if this reference is the last instance
	void dereference( const std::string& path_);

private:
	utils::Mutex m_map_mutex;
	std::map<std::string,utils::SharedPtr<LevelDbHandle> > m_map;
};


/// \brief Implementation of the strus key value storage database based on the LevelDB library
class DatabaseClient
	:public DatabaseClientInterface
{
public:
	/// \brief Constructor
	/// \param[in] dbmap_ reference to map of shared levelDB handles
	/// \param[in] path path of the storage
	/// \param[in] maxOpenFiles maximum number of files open (0 for default)
	/// \param[in] cachesize_k number of K LRU cache for nodes
	/// \param[in] compression wheter to use snappy compression (true) or not
	/// \param[in] writeBufferSize size of write buffer per file
	/// \param[in] blockSize block size on disk (size of units)
	DatabaseClient(
			const utils::SharedPtr<LevelDbHandleMap>& dbmap_,
			const std::string& path,
			unsigned int maxOpenFiles,
			unsigned int cachesize_k,
			bool compression,
			unsigned int writeBufferSize,
			unsigned int blockSize,
			ErrorBufferInterface* errorhnd_)
		:m_dbmap(dbmap_),m_db(dbmap_->create( path, maxOpenFiles, cachesize_k, compression, writeBufferSize, blockSize)),m_errorhnd(errorhnd_)
	{}

	virtual ~DatabaseClient();

	virtual DatabaseTransactionInterface* createTransaction();

	virtual DatabaseCursorInterface* createCursor( const DatabaseOptions& options) const;

	virtual DatabaseBackupCursorInterface* createBackupCursor() const;
	
	virtual void writeImm(
			const char* key,
			std::size_t keysize,
			const char* value,
			std::size_t valuesize);

	virtual void removeImm(
			const char* key,
			std::size_t keysize);

	virtual bool readValue(
			const char* key,
			std::size_t keysize,
			std::string& value,
			const DatabaseOptions& options) const;

private:
	utils::SharedPtr<LevelDbHandleMap> m_dbmap;		///< reference to map of shared levelDB handles
	utils::SharedPtr<LevelDbHandle> m_db;			///< shared levelDB handle
	ErrorBufferInterface* m_errorhnd;			///< buffer for reporting errors
};

}//namespace
#endif


