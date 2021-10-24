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
		Iterator();
		Iterator( const std::string& directory_, std::size_t prefixsize_, const std::vector<std::string>& filelist_);
		Iterator( const Iterator& o);

		TimeStamp timestamp() const
			{return m_timestamp;}
		bool defined() const
			{return m_timestamp >= 0;}

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

	class TimeStampIterator
	{
	public:
		TimeStampIterator();
		TimeStampIterator( std::size_t prefixsize_, const std::vector<std::string>& filelist_);
		TimeStampIterator( const TimeStampIterator& o);

		TimeStamp timestamp() const
			{return m_timestamp;}

		TimeStamp next();

	private:
		TimeStamp m_timestamp;
		std::vector<TimeStamp> m_ar;
		std::vector<TimeStamp>::const_iterator m_itr;
	};

public:
	/// \brief Constructor
	/// \param[in] directory_ where to store the files
	/// \param[in] prefix_ filename prefix to use for files stored
	/// \param[in] extension_ extension to use for files stored
	DatedFileList( const std::string& directory_, const std::string& prefix_, const std::string& extension_);

	/// \brief Copy constructor
	DatedFileList( const DatedFileList& o);

	/// \brief Stores a blobs to files in the context of a transaction (operation either fails entirely or succeeds completely)
	/// \param[in] blobs data to store
	/// \param[in] tmpfileext extension to use for temporary files
	/// \note Transaction completed by file renames, a failed store may lead to files with this extension laying around.
	void store( const std::vector<std::string>& blobs, const char* tmpfileext);

	Iterator getIterator( const TimeStamp timestamp) const;
	TimeStampIterator getTimeStampIterator( const TimeStamp timestamp) const;

	/// \brief Load the blob associated with the timestamp specified
	std::string loadBlob( const TimeStamp timestamp) const;

	/// \brief Deletes all files before a certain date/time
	/// \note Used for cleanup
	void deleteFilesBefore( const TimeStamp timestamp);

private:
	std::string newFileName();
	void createWorkingDirectoryIfNotExist();
	std::vector<std::string> getFileNames( const TimeStamp timestamp) const;

private:
	std::string m_directory;
	std::string m_prefix;
	std::string m_extension;
};

} //namespace
#endif
