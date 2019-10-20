/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Library with some convenient functions to create storage objects
/// \file "storage_objbuild.hpp"
#ifndef _STRUS_STORAGE_OBJBUILD_LIB_HPP_INCLUDED
#define _STRUS_STORAGE_OBJBUILD_LIB_HPP_INCLUDED
#include <string>

/// \brief strus toplevel namespace
namespace strus {

/// \brief Forward declaration
class StorageObjectBuilderInterface;
/// \brief Forward declaration
class StorageAlterMetaDataTableInterface;
/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class VectorStorageClientInterface;
/// \brief Forward declaration
class VectorStorageBuilderInterface;
/// \brief Forward declaration
class ErrorBufferInterface;
/// \brief Forward declaration
class FileLocatorInterface;

///\brief Create a storage object builder with the builders from the standard strus core libraries (without module support)
///\param[in] filelocator interface to get paths of resource files and the working directory
///\param[in] errorhnd error buffer interface
StorageObjectBuilderInterface*
	createStorageObjectBuilder_default(
		const FileLocatorInterface* filelocator,
		ErrorBufferInterface* errorhnd);

///\brief Create a storage client interface with the object builder passed
///\param[in] objbuilder object builder
///\param[in] errorhnd error buffer interface
///\param[in] config object configuration (source, not a filename)
StorageClientInterface*
	createStorageClient(
		const StorageObjectBuilderInterface* objbuilder,
		ErrorBufferInterface* errorhnd,
		const std::string& config);

///\brief Create a vector storage client interface with the object builder passed
///\param[in] objbuilder object builder
///\param[in] errorhnd error buffer interface
///\param[in] config object configuration (source, not a filename)
VectorStorageClientInterface*
	createVectorStorageClient(
		const StorageObjectBuilderInterface* objbuilder,
		ErrorBufferInterface* errorhnd,
		const std::string& config);

}//namespace
#endif

