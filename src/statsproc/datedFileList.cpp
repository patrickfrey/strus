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
#include "strus/base/stdint.h"
#include "strus/base/fileio.hpp"
#include "strus/base/atomic.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/string_conv.hpp"
#include "private/internationalization.hpp"
#include <string>
#include <cstring>
#include <vector>
#include <ctime>

using namespace strus;

static bool onlyDigits( char const* ti)
{
	for (;*ti >= '0' && *ti <= '9'; ++ti){}
	return !*ti;
}

static TimeStamp parseTimeStamp( const char* timestampstr)
{
	char const* ti = timestampstr;
	struct tm date;

	char YY_buf[ 5];
	std::memcpy( YY_buf, ti, 4); YY_buf[4] = 0; ti += 4;
	char mm_buf[ 3];
	std::memcpy( mm_buf, ti, 2); mm_buf[2] = 0; ti += 2;
	char dd_buf[ 3];
	std::memcpy( dd_buf, ti, 2); dd_buf[2] = 0; ti += 2;
	if (*ti != '_') throw std::runtime_error( _TXT("timestamp parse error"));
	ti += 1;
	char HH_buf[ 3];
	std::memcpy( HH_buf, ti, 2); HH_buf[2] = 0; ti += 2;
	char MM_buf[ 3];
	std::memcpy( MM_buf, ti, 2); MM_buf[2] = 0; ti += 2;
	char SS_buf[ 3];
	std::memcpy( SS_buf, ti, 2); SS_buf[2] = 0; ti += 2;
	if (*ti != '_') throw std::runtime_error( _TXT("timestamp parse error"));
	ti += 1;
	char ii_buf[ 5];
	std::memcpy( ii_buf, ti, 4); ii_buf[4] = 0; ti += 4;

	if (!onlyDigits( YY_buf)
	||  !onlyDigits( mm_buf)
	||  !onlyDigits( dd_buf)
	||  !onlyDigits( HH_buf)
	||  !onlyDigits( MM_buf)
	||  !onlyDigits( SS_buf)
	||  !onlyDigits( ii_buf)) throw std::runtime_error( _TXT("timestamp parse error"));

	std::memset( &date, 0, sizeof(date));
	date.tm_sec  = ::atoi(SS_buf);
	date.tm_min  = ::atoi(MM_buf);
	date.tm_hour = ::atoi(HH_buf);
	date.tm_mday = ::atoi(dd_buf);
	date.tm_mon  = ::atoi(mm_buf)-1;
	date.tm_year = ::atoi(YY_buf) - 1900;
	date.tm_isdst = 0;

	int counter = ::atoi( ii_buf);
	time_t unixtime = ::mktime( &date);

	if (unixtime == ((time_t)-1))
	{
		throw std::runtime_error( _TXT("timestamp syntax error"));
	}
	else
	{
		return TimeStamp( unixtime, counter);
	}
}

static std::string fileNameFromTimeStamp( const std::string& prefix, const TimeStamp& timestamp, const std::string& extension)
{
	char timebuf[ 256];
	char idxbuf[ 32];

	time_t tt = timestamp.unixtime();
	const struct tm* tm_info = ::localtime( &tt);

	std::strftime( timebuf, sizeof(timebuf), "%Y%m%d_%H%M%S", tm_info);
	std::snprintf( idxbuf, sizeof(idxbuf), "%04d", timestamp.counter());
	return strus::string_format( "%s%s_%s%s", prefix.c_str(), timebuf, idxbuf, extension.c_str());
}

DatedFileList::DatedFileList( const std::string& directory_, const std::string& prefix_, const std::string& extension_)
	:m_mutex(),m_timestamp(0,0),m_directory(directory_),m_prefix(prefix_),m_extension(extension_)
{
	if (!m_directory.empty()) createWorkingDirectoryIfNotExist();
}

std::string DatedFileList::newFileName()
{
	return strus::joinFilePath( m_directory, fileNameFromTimeStamp( m_prefix, allocTimestamp(), m_extension));
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

TimeStamp DatedFileList::allocTimestamp()
{
	time_t current_time = ::time(NULL);
	if (current_time == ((time_t)-1)) throw std::runtime_error( _TXT( "failed to get system time"));

	scoped_lock lock( m_mutex);
	if (current_time == m_timestamp.unixtime())
	{
		m_timestamp = TimeStamp( current_time, m_timestamp.counter()+1);
	}
	else
	{
		m_timestamp = TimeStamp( current_time);
	}
	return m_timestamp;
}

TimeStamp DatedFileList::currentTimestamp()
{
	strus::scoped_lock lock( m_mutex);
	if (m_timestamp.unixtime() == 0)
	{
		time_t current_time = ::time(NULL);
		if (current_time == ((time_t)-1)) throw std::runtime_error( _TXT( "failed to get system time"));
		return TimeStamp( current_time-1, 0);
	}
	else
	{
		return m_timestamp;
	}
}

void DatedFileList::Iterator::loadBlob()
{
	if (m_fileiter == m_filelist.end())
	{
		m_blob.clear();
	}
	else
	{
		int ec = strus::readFile( strus::joinFilePath( m_directory, *m_fileiter), m_blob);
		m_timestamp = parseTimeStamp( m_fileiter->c_str() + m_prefixsize);
		if (ec != 0) throw std::runtime_error(::strerror(ec));
	}
}

DatedFileList::Iterator::Iterator( const std::string& directory_, std::size_t prefixsize_, const std::vector<std::string>& filelist_, const TimeStamp& timestamp_)
	:m_timestamp(timestamp_),m_blob(),m_directory(directory_),m_prefixsize(prefixsize_),m_filelist(filelist_)
{
	m_fileiter = m_filelist.begin();
	loadBlob();
}

DatedFileList::Iterator::Iterator( const TimeStamp& timestamp_)
	:m_timestamp(timestamp_),m_blob(),m_directory(),m_prefixsize(0),m_filelist()
{
	m_fileiter = m_filelist.begin();
}

DatedFileList::Iterator::Iterator( const Iterator& o)
	:m_timestamp(o.m_timestamp),m_blob(o.m_blob),m_directory(o.m_directory),m_prefixsize(o.m_prefixsize),m_filelist(o.m_filelist)
{
	m_fileiter = m_filelist.begin() + (o.m_fileiter - o.m_filelist.begin());
}

bool DatedFileList::Iterator::next()
{
	if (m_directory.empty()) return false;
	if (m_fileiter == m_filelist.end()) return false;
	++m_fileiter;
	loadBlob();
	return true;
}

DatedFileList::Iterator DatedFileList::getIterator( const TimeStamp& timestamp_)
{
	if (m_directory.empty())
	{
		return Iterator( timestamp_);
	}
	std::vector<std::string> filtered_files;
	std::vector<std::string> files;
	TimeStamp timestamp_current = currentTimestamp();
	int ec = strus::readDirFiles( m_directory, m_extension, files);
	if (ec != 0) throw std::runtime_error(::strerror(ec));

	std::string filterfilename_start = fileNameFromTimeStamp( m_prefix, timestamp_, m_extension);
	std::string filterfilename_end = fileNameFromTimeStamp( m_prefix, timestamp_current, m_extension);
	std::vector<std::string>::const_iterator fi = files.begin(), fe = files.end();
	for (; fi != fe; ++fi)
	{
		if (strus::stringStartsWith( *fi, m_prefix) && *fi > filterfilename_start && *fi <= filterfilename_end)
		{
			filtered_files.push_back( *fi);
		}
	}
	return Iterator( m_directory, m_prefix.size(), filtered_files, timestamp_);
}

void DatedFileList::deleteFilesBefore( const TimeStamp& timestamp)
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
			strus::removeFile( strus::joinFilePath( m_directory, *fi));
		}
	}
}

