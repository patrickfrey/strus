/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013 Patrick Frey

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
/// \brief Exported functions of the strus storage library
#ifndef _STRUS_STORAGE_HPP_INCLUDED
#define _STRUS_STORAGE_HPP_INCLUDED
#include "strus/storageInterface.hpp"

namespace strus {

/// \brief Creates an instance of the storage interface described with config
/// \remark Because of restrictions imposed by LevelDB only one instance of a storage can be crated per storage
StorageInterface* createStorageClient( const char* config);

/// \brief Creates a new storage described with config in the file system
void createStorageDatabase( const char* config);

/// \brief Gets an example configuration description for the functions
///	createStorageDatabase(const char*) and createStorageClient(const char*)
///	for the usage printed by programs using the storage.
const char* getStorageConfigDescription();

}//namespace
#endif

