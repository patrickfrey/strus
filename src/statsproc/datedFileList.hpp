/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief List of files with datetime stamp suitable to store and retrieve data dump snapshots from a certain date
/// \file datedFileList.hpp
#ifndef _STRUS_STORAGE_DATED_FILE_LIST_HPP_INCLUDED
#define _STRUS_STORAGE_DATED_FILE_LIST_HPP_INCLUDED
#include "strus/base/atomic.hpp"
#include "strus/base/thread.hpp"
#include "strus/timeStamp.hpp"
#include <string>
#include <vector>

namespace strus
{

class DatedFileList
{
public:
	class Iterator
	{
	public:
		Iterator( const std::string& directory_, std::size_t prefixsize_, const std::vector<std::string>& filelist_, const TimeStamp& timestamp_);
		Iterator( const Iterator& o)
			:m_timestamp(o.m_timestamp),m_blob(o.m_blob),m_directory(o.m_directory),m_prefixsize(o.m_prefixsize),m_filelist(o.m_filelist)
		{
			m_fileiter = m_filelist.begin() + (o.m_fileiter - o.m_filelist.begin());
		}

		const TimeStamp& timestamp() const
		{
			return m_timestamp;
		}

		bool next();

		const void* blob() const
			{return m_blob.c_str();}
		std::size_t blobsize() const
			{return m_blob.size();}

	private:
		TimeStamp m_timestamp;
		std::string m_blob;
		std::string m_directory;
		std::size_t m_prefixsize;
		std::vector<std::string> m_filelist;
		std::vector<std::string>::const_iterator m_fileiter;
	};

public:
	DatedFileList( const std::string& directory_, const std::string& prefix_, const std::string& extension_)
		:m_mutex(),m_timestamp(0,0),m_directory(directory_),m_prefix(prefix_),m_extension(extension_){}

	// \brief Stores a blob to file
	// \return errno if failed, 0 on success
	void store( const void* binblob, std::size_t binblobsize);

	TimeStamp currentTimestamp();
	TimeStamp allocTimestamp();

	Iterator getIterator( const TimeStamp& timestamp);
	void deleteFilesBefore( TimeStamp& timestamp);

private:
	std::string newFileName();
	void createWorkingDirectoryIfNotExist();

private:
	strus::mutex m_mutex;
	TimeStamp m_timestamp;
	std::string m_directory;
	std::string m_prefix;
	std::string m_extension;
};

} //namespace
#endif
