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

	/// \brief Get the smallest existing timestamp that is bigger or equal to the specified timestamp
	TimeStamp getUpperBoundTimeStamp( const TimeStamp timestamp) const;

	/// \brief Load the blob associated with the timestamp specified
	std::string loadBlob( const TimeStamp timestamp) const;

	/// \brief Deletes all files before a certain date/time
	/// \note Used for cleanup
	void deleteFilesBefore( const TimeStamp timestamp);

private:
	std::string newFileName();
	void createWorkingDirectoryIfNotExist();
	std::string getUpperBoundFile( const TimeStamp timestamp) const;

private:
	std::string m_directory;
	std::string m_prefix;
	std::string m_extension;
};

} //namespace
#endif
