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
		explicit Iterator( const TimeStamp& timestamp_);
		Iterator( const std::string& directory_, std::size_t prefixsize_, const std::vector<std::string>& filelist_, const TimeStamp& timestamp_);
		Iterator( const Iterator& o);

		const TimeStamp& timestamp() const
			{return m_timestamp;}

		const void* blob() const
			{return m_blob.c_str();}
		std::size_t blobsize() const
			{return m_blob.size();}

		bool next();

	private:
		void loadBlob();

	private:
		TimeStamp m_timestamp;
		std::string m_blob;
		std::string m_directory;
		std::size_t m_prefixsize;
		std::vector<std::string> m_filelist;
		std::vector<std::string>::const_iterator m_fileiter;
	};

public:
	/// \brief Constructor
	/// \param[in] directory_ where to store the files
	/// \param[in] prefix_ filename prefix to use for files stored
	/// \param[in] extension_ extension to use for files stored
	DatedFileList( const std::string& directory_, const std::string& prefix_, const std::string& extension_);

	/// \brief Stores a blobs to files in the context of a transaction (operation either fails entirely or succeeds completely)
	/// \param[in] blobs data to store
	/// \param[in] tmpfileext extension to use for temporary files
	/// \note Transaction completed by file renames, a failed store may lead to files with this extension laying around.
	void store( const std::vector<std::string>& blobs, const char* tmpfileext);

	TimeStamp currentTimestamp();
	TimeStamp allocTimestamp();

	Iterator getIterator( const TimeStamp& timestamp);

	/// \brief Deletes all files before a certain date/time
	/// \note Used for cleanup
	void deleteFilesBefore( const TimeStamp& timestamp);

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
