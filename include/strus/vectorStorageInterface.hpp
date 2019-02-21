/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for vector storage operations needed for efficient similar vector retrieval in the strus storage
#ifndef _STRUS_VECTOR_STORGE_INTERFACE_HPP_INCLUDED
#define _STRUS_VECTOR_STORGE_INTERFACE_HPP_INCLUDED
#include <bitset>
#include <vector>

namespace strus {

/// \brief Forward declaration
class VectorStorageBuilderInterface;
/// \brief Forward declaration
class VectorStorageClientInterface;
/// \brief Forward declaration
class VectorStorageDumpInterface;
/// \brief Forward declaration
class DatabaseInterface;


/// \brief Interface for storing an retrieving vectors of floating point numbers representing word embeddings.
class VectorStorageInterface
{
public:
	/// \brief Destructor
	virtual ~VectorStorageInterface(){}

	/// \brief Create a data repository for the data of a vector storage
	/// \param[in] configsource configuration string of the storage and the database (not a filename!)
	/// \param[in] database database type for the repository
	/// \return true on success, false if an error occurred
	virtual bool createStorage(
			const std::string& configsource,
			const DatabaseInterface* database) const=0;

	/// \brief Create a new vector storage client interface
	/// \param[in] configsource configuration string of the storage and the database (not a filename!)
	/// \param[in] database database type of the persistent storage
	/// \return the client interface (with ownership)
	/// \remark The repository refered to by the configuration must exist and must have been built (-> createRepository)
	virtual VectorStorageClientInterface* createClient(
			const std::string& configsource,
			const DatabaseInterface* database) const=0;

	/// \brief Create a dump of the contents of a vector storage
	/// \param[in] configsource Configuration source string describing the storage and the database (not a filename !)
	/// \param[in] database key value store database type used by the vector storage
	/// \return the object to fetch the dump from
	virtual VectorStorageDumpInterface* createDump(
			const std::string& configsource,
			const DatabaseInterface* database) const=0;

	/// \brief Enumeration of different type of configurations
	///	Needed for getting the correct description of the configuration
	enum ConfigType
	{
		CmdCreateClient,		///< Config description for the creation of an instance accessing the repository
		CmdCreate			///< Config description for the creation of a repository that does not exist yet
	};

	/// \brief Gets a configuration description (source string as used by the functions here)
	///	createStorage(const std::string&) and createClient(const std::string&)
	///	for the usage printed by programs using this storage implementation.
	virtual const char* getConfigDescription( const ConfigType& type) const=0;

	/// \brief Get the list of known configuration parameter keys
	///	for verification of the configuration by programs using this storage implementation.
	virtual const char** getConfigParameters( const ConfigType& type) const=0;
};

}//namespace
#endif


