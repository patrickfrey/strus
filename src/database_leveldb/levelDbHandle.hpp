/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_DATABASE_LEVELDB_HANDLE_HPP_INCLUDED
#define _STRUS_DATABASE_LEVELDB_HANDLE_HPP_INCLUDED
#include "strus/base/shared_ptr.hpp"
#include "strus/base/thread.hpp"
#include <leveldb/db.h>
#include <vector>
#include <string>
#include <list>

namespace strus
{

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
	std::string config() const;

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

typedef strus::shared_ptr<LevelDbHandle> LevelDbHandleRef;


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
	strus::shared_ptr<LevelDbHandle> create(
			const std::string& path_,
			unsigned int maxOpenFiles,
			unsigned int cachesize_k,
			bool compression,
			unsigned int writeBufferSize_,
			unsigned int blockSize_);

	/// \brief Dereference the handle for the database referenced by path and dispose the handle, if this reference is the last instance
	void dereference( const char* path_);

private:
	strus::mutex m_map_mutex;					///< mutex for monitor of m_map
	std::vector<LevelDbHandleRef> m_map;				///< implementation of a map as vector (because only few elements < 10 expected)
};

/// \brief DB Connection handle
class LevelDbConnection
{
public:
	LevelDbConnection( LevelDbHandleMap* dbmap_, const strus::shared_ptr<LevelDbHandle>& db_)
		:m_dbmap(dbmap_),m_db(db_){}
	LevelDbConnection( const LevelDbConnection& o)
		:m_dbmap(o.m_dbmap),m_db(o.m_db){}

	~LevelDbConnection()
	{
		close();
	}

	void close();

	leveldb::DB* db() const
	{
		LevelDbHandle* hnd = m_db.get();
		if (!hnd) return 0;
		return hnd->db();
	}

	std::string config() const
	{
		return (m_db.get())?m_db->config():std::string();
	}

	std::string path() const
	{
		return (m_db.get())?m_db->path():std::string();
	}

private:
	LevelDbHandleMap* m_dbmap;			///< pointer to map of shared levelDB handles, needed for unregister
	strus::shared_ptr<LevelDbHandle> m_db;		///< shared levelDB handle
};


/// \brief DB Connection handle with an interator
class LevelDbIterator
	:public LevelDbConnection
{
public:
	LevelDbIterator( const leveldb::ReadOptions& opt_, const LevelDbConnection& conn);
	~LevelDbIterator();

	leveldb::Iterator* itr()			{return m_itr;}
	const leveldb::Iterator* itr() const		{return m_itr;}
	const leveldb::ReadOptions& opt() const		{return m_opt;}

	void done();

private:
#if __cplusplus >= 201103L
	LevelDbIterator( LevelDbIterator&) = delete;
	void operator=( LevelDbIterator&) = delete;
#endif
private:
	leveldb::Iterator* m_itr;
	leveldb::ReadOptions m_opt;
};

}//namespace
#endif


