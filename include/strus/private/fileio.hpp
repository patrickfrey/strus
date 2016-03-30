/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
/// \param[in] ext the file extension the accepted files must have (empty if all extensions are accepted)
/// \param[out] res the returned matching file names of the directory read
/// \return 0 on success, errno on failure
unsigned int readDirFiles( const std::string& path, const std::string& ext, std::vector<std::string>& res);

/// \brief Reads all subdirectories of a directory to an array of strings
/// \param[in] path the name of director to read
/// \param[out] res the returned matching sub directories of the directory read
/// \return 0 on success, errno on failure
unsigned int readDirSubDirs( const std::string& path, std::vector<std::string>& res);

/// \brief Check if a file system path points to a file
/// \return true, if yes
/// \param[in] path file system path to check
bool isFile( const std::string& path);

/// \brief Check if a file system path points to a directory
/// \param[in] path file system path to check
/// \return true, if yes
bool isDir( const std::string& path);

/// \brief Get the parent (containing) path of a file or directory
/// \param[in] path file system path to get the containing path
/// \param[out] dest the containing path
/// \return 0 on success, errno on failure
unsigned int getParentPath( const std::string& path, std::string& dest);

/// \brief Get the OS path element separator
/// \return the separator
char dirSeparator();

}
#endif

