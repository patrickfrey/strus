/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "strus/private/fileio.hpp"
#include "private/dll_tags.hpp"
#include "fileio_sys.hpp"
#include <cstdio>
#include <cerrno>
#include <dirent.h>
#include <sys/stat.h>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <functional>

using namespace strus;

DLL_PUBLIC unsigned int strus::writeFile( const std::string& filename, const std::string& content)
{
	unsigned char ch;
	FILE* fh = ::fopen( filename.c_str(), "wb");
	if (!fh)
	{
		return errno;
	}
	std::string::const_iterator fi = content.begin(), fe = content.end();
	for (; fi != fe; ++fi)
	{
		ch = *fi;
		if (1 > ::fwrite( &ch, 1, 1, fh))
		{
			int ec = ::ferror( fh);
			if (ec)
			{
				::fclose( fh);
				return ec;
			}
		}
	}
	::fclose( fh);
	return 0;
}

DLL_PUBLIC unsigned int strus::readFile( const std::string& filename, std::string& res)
{
	FILE* fh = ::fopen( filename.c_str(), "rb");
	if (!fh)
	{
		return errno;
	}
	unsigned int nn;
	enum {bufsize=(1<<12)};
	char buf[ bufsize];

	while (!!(nn=::fread( buf, 1, bufsize, fh)))
	{
		try
		{
			res.append( buf, nn);
		}
		catch (const std::bad_alloc&)
		{
			::fclose( fh);
			return 12/*ENOMEM*/;
		}
	}
	if (!feof( fh))
	{
		unsigned int ec = ::ferror( fh);
		::fclose( fh);
		return ec;
	}
	else
	{
		::fclose( fh);
	}
	return 0;
}

DLL_PUBLIC unsigned int strus::readStdin( std::string& res)
{
	unsigned int nn;
	enum {bufsize=(1<<12)};
	char buf[ bufsize];

	while (!!(nn=::fread( buf, 1, bufsize, stdin)))
	{
		try
		{
			res.append( buf, nn);
		}
		catch (const std::bad_alloc&)
		{
			return 12/*ENOMEM*/;
		}
	}
	return 0;
}

DLL_PUBLIC unsigned int strus::readDirSubDirs( const std::string& path, std::vector<std::string>& res)
{
	DIR *dir = ::opendir( path.c_str());
	struct dirent *ent;

	if (!dir)
	{
		return errno;
	}
	try
	{
		std::size_t prevsize = res.size();
		while (!!(ent = ::readdir(dir)))
		{
			if (ent->d_name[0] == '.') continue;
			std::string entry( path + dirSeparator() + ent->d_name);
			if (isDir( entry))
			{
				res.push_back( ent->d_name);
			}
		}
		std::sort( res.begin()+prevsize, res.end(), std::less<std::string>());
		unsigned int err = ::closedir(dir);
		if (err)
		{
			return err;
		}
	}
	catch (const std::bad_alloc&)
	{
		::closedir(dir);
		return 12/*ENOMEM*/;
	}
	return 0;
}

DLL_PUBLIC unsigned int strus::readDirFiles( const std::string& path, const std::string& ext, std::vector<std::string>& res)
{
	DIR *dir = ::opendir( path.c_str());
	struct dirent *ent;

	if (!dir)
	{
		return errno;
	}
	try
	{
		std::size_t prevsize = res.size();
		while (!!(ent = ::readdir(dir)))
		{
			if (ent->d_name[0] == '.') continue;
			std::string entry( ent->d_name);
			if (ext.size() > entry.size())
			{
				continue;
			}
			std::string entrypath( path + dirSeparator() + entry);
			if (isDir( entrypath)) continue;
			if (ext.empty())
			{
				res.push_back( entry);
			}
			else
			{
				const char* ee = entry.c_str() + entry.size() - ext.size();
				if (entry[0] != '.' && 0==std::memcmp( ee, ext.c_str(), ext.size()))
				{
					res.push_back( entry );
				}
			}
		}
		std::sort( res.begin()+prevsize, res.end(), std::less<std::string>());
		unsigned int err = ::closedir(dir);
		return err;
	}
	catch (const std::bad_alloc&)
	{
		::closedir(dir);
		return 12/*ENOMEM*/;
	}
}

enum PathType {PathFile,PathDir,PathUnknown,PathError};
static PathType getPathType( const std::string& path)
{
	struct stat s;
	if (::stat( path.c_str(), &s) == 0)
	{
		if( s.st_mode & S_IFDIR )
		{
			return PathDir;
		}
		else if( s.st_mode & S_IFREG )
		{
			return PathFile;
		}
		else
		{
			return PathUnknown;
		}
	}
	else
	{
		return PathError;
	}
}

DLL_PUBLIC bool strus::isFile( const std::string& path)
{
	return getPathType(path)==PathFile;
}

DLL_PUBLIC bool strus::isDir( const std::string& path)
{
	return getPathType(path)==PathDir;
}

DLL_PUBLIC char strus::dirSeparator()
{
	return STRUS_FILEIO_DIRSEP;
}

DLL_PUBLIC unsigned int strus::getParentPath( const std::string& path, std::string& dest)
{
	const char* ri = path.c_str();
	char const* re = path.c_str()+path.size();
	for (; re != ri && *(re-1) == STRUS_FILEIO_DIRSEP; --re){}
	for (; re != ri && *(re-1) != STRUS_FILEIO_DIRSEP; --re){}
	if (re == ri)
	{
		dest.clear();
		return 0;
	}
	else
	{
		for (; re != ri && *(re-1) == STRUS_FILEIO_DIRSEP; --re){}
		try
		{
			dest.clear();
			dest.append( ri, re-ri-1);
		}
		catch (const std::bad_alloc&)
		{
			return 12/*ENOMEM*/;
		}
		dest.clear();
		return 0;
	}
}

