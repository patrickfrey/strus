/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#ifndef _STRUS_FILE_IO_HPP_INCLUDED
#define _STRUS_FILE_IO_HPP_INCLUDED
#include <vector>
#include <string>

namespace strus
{

/// \brief Reads the complete content of a file to a string
/// \param[in] filename the name of file to read
/// \param[out] res the returned content of the file read
/// \return 0 on success, errno on failure
unsigned int readFile( const std::string& filename, std::string& res);

/// \brief Reads the complete input of stdin to a string
/// \param[out] res the returned content of the file read
/// \return 0 on success, errno on failure
unsigned int readStdin( std::string& res);

/// \brief Writes a string to a file creating the file if it does not exist
/// \param[in] filename the name of the file to write
/// \param[out] content content of the string to write
/// \return 0 on success, errno on failure
unsigned int writeFile( const std::string& filename, const std::string& content);

/// \brief Reads all file names matching to the extension ext of a directory to an array of strings
/// \param[in] path the name of director to read
/// \param[in] ext the file extension the accepted files must have
/// \param[out] res the returned matching file names of the directory read
/// \return 0 on success, errno on failure
unsigned int readDir( const std::string& path, const std::string& ext, std::vector<std::string>& res);

/// \brief Check if a file system path points to a file
/// \return true, if yes
/// \param[in] path file system path to check
bool isFile( const std::string& path);

/// \brief Check if a file system path points to a directory
/// \param[in] path file system path to check
/// \return true, if yes
bool isDir( const std::string& path);

/// \brief Get the OS path element separator
/// \return the separator
char dirSeparator();

}
#endif

