/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief List of files with datetime stamp suitable to store and retrieve data dump snapshots from a certain date
/// \file datedFileList.cpp
#include "datedFileList.hpp"
#include "strus/lib/error.hpp"
#include "strus/base/stdint.h"
#include "strus/base/fileio.hpp"
#include "strus/base/atomic.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/string_conv.hpp"
#include "strus/timeStamp.hpp"
#include "private/internationalization.hpp"
#include <string>
#include <cstring>
#include <vector>
#include <ctime>
#include <algorithm>

using namespace strus;

static std::string fileNameFromTimeStamp( const std::string& prefix, const TimeStamp timestamp, const std::string& extension)
{
	TimeStampString timestampstr = timeStampToString( timestamp);
	if (!timestampstr.str[0]) throw std::runtime_error( _TXT("bad timestamp"));
	return prefix + timestampstr.str + extension;
}

DatedFileList::DatedFileList( const std::string& directory_, const std::string& prefix_, const std::string& extension_)
	:m_directory(directory_),m_prefix(prefix_),m_extension(extension_)
{
	if (!m_directory.empty()) createWorkingDirectoryIfNotExist();
}

DatedFileList::DatedFileList( const DatedFileList& o)
	:m_directory(o.m_directory),m_prefix(o.m_prefix),m_extension(o.m_extension)
{}

std::string DatedFileList::newFileName()
{
	TimeStamp newTimeStamp = getCurrentTimeStamp();

	std::string filename = fileNameFromTimeStamp( m_prefix, newTimeStamp, m_extension);
	std::string rt = strus::joinFilePath( m_directory, filename);
	if (rt.empty()) throw std::bad_alloc();
	return rt;
}

void DatedFileList::createWorkingDirectoryIfNotExist()
{
	(void)strus::createDir( m_directory, false/*fail if exist*/);
}

static void deleteFileList( const std::vector<std::string>& files)
{
	std::vector<std::string>::const_iterator fi = files.begin(), fe = files.end();
	for (; fi != fe; ++fi)
	{
		int ec = strus::removeFile( *fi, false);
		if (ec != 0)
		{
			throw std::runtime_error( ::strerror(ec));
			//... if this fails we are really fucked
		}
	}
}

static bool moveFileList( const std::vector<std::string>& files, const char* tmpext)
{
	std::vector<std::string>::const_iterator fi = files.begin(), fe = files.end();
	for (; fi != fe; ++fi)
	{
		int ec;
		try
		{
			std::string tmpfilename( *fi + tmpext);
			ec = strus::renameFile( tmpfilename, *fi);
		}
		catch (...)
		{
			ec = 12/*ENOMEM*/;
		}
		if (ec != 0)
		{
			return false;
		}
	}
	return true;
}

void DatedFileList::store( const std::vector<std::string>& blobs, const char* tmpext)
{
	if (m_directory.empty())
	{
		throw std::runtime_error( _TXT("failed to store files because no directory defined"));
	}
	std::vector<std::string> written_files;
	std::string tmpfilename;
	std::vector<std::string>::const_iterator bi = blobs.begin(), be = blobs.end();
	for (; bi != be; ++bi)
	{
		try
		{
			std::string filename = DatedFileList::newFileName();
			tmpfilename = filename + tmpext;
			written_files.push_back( filename);
		}
		catch (const std::bad_alloc&)
		{
			deleteFileList( written_files);
			throw std::bad_alloc();
		}
		int ec = strus::writeFile( tmpfilename, *bi);
		if (ec)
		{
			deleteFileList( written_files);
			throw std::runtime_error(::strerror(ec));
		}
	}
	if (!moveFileList( written_files, tmpext))
	{
		deleteFileList( written_files);
		throw std::runtime_error( _TXT("failed to move files in dated file list store transaction"));
	}
}

std::string DatedFileList::getUpperBoundFile( const TimeStamp timestamp) const
{
	std::string rt;
	if (m_directory.empty())
	{
		return rt;
	}
	std::vector<std::string> files;
	TimeStamp timestamp_current = getCurrentTimeStamp();

	int ec = strus::readDirFiles( m_directory, m_extension, files);
	if (ec != 0) throw std::runtime_error(::strerror(ec));
	std::sort( files.begin(), files.end());

	std::string filterfilename_start = timestamp >= 0 ? fileNameFromTimeStamp( m_prefix, timestamp, m_extension) : std::string();
	std::string filterfilename_end = fileNameFromTimeStamp( m_prefix, timestamp_current, m_extension);
	std::vector<std::string>::const_iterator fi = files.begin(), fe = files.end();
	for (; fi != fe; ++fi)
	{
		if (strus::stringStartsWith( *fi, m_prefix) && *fi >= filterfilename_start && *fi <= filterfilename_end)
		{
			rt = *fi;
		}
	}
	return rt;
}

TimeStamp DatedFileList::getUpperBoundTimeStamp( const TimeStamp timestamp) const
{
	std::string file = getUpperBoundFile( timestamp);
	TimeStamp rt = timeStampFromString( file.c_str() + m_prefix.size());
	if (rt < 0) throw std::runtime_error( _TXT("bad filename format, could not extract timestamp"));
	return rt;
}

void DatedFileList::deleteFilesBefore( const TimeStamp timestamp)
{
	if (m_directory.empty())
	{
		return;
	}
	std::vector<std::string> files;
	int ec = strus::readDirFiles( m_directory, m_extension, files);
	if (ec != 0) throw std::runtime_error(::strerror(ec));

	std::string filterfilename = fileNameFromTimeStamp( m_prefix, timestamp, m_extension);
	std::vector<std::string>::const_iterator fi = files.begin(), fe = files.end();
	for (; fi != fe; ++fi)
	{
		if (strus::stringStartsWith( *fi, m_prefix) && *fi <= filterfilename)
		{
			std::string filepath = strus::joinFilePath( m_directory, *fi);
			if (filepath.empty()) throw std::bad_alloc();
			(void)strus::removeFile( filepath);
		}
	}
}

std::string DatedFileList::loadBlob( const TimeStamp timestamp) const
{
	if (timestamp < 0) return std::string();
	std::string filename = fileNameFromTimeStamp( m_prefix, timestamp, m_extension);
	std::string filepath = strus::joinFilePath( m_directory, filename);
	if (filepath.empty()) throw std::bad_alloc();
	std::string blob;
	int ec = strus::readFile( filepath, blob);
	if (ec != 0) throw std::runtime_error(::strerror(ec));
	return blob;
}

