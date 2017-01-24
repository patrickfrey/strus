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


/// \brief Interface for storing an retrieving vectors of floating point numbers representing document features and a model relating them to concept features. The relation is learnt by the model builder in an unsupervised way.
class VectorStorageInterface
{
public:
	/// \brief Destructor
	virtual ~VectorStorageInterface(){}

	/// \brief Create a data repository for the data of a vector storage
	/// \param[in] configsource configuration string of the model and the database (not a filename!)
	/// \param[in] database database type for the repository
	/// \return true on success, false if an error occurred
	virtual bool createStorage(
			const std::string& configsource,
			const DatabaseInterface* database) const=0;

	/// \brief Reset an existing repository for the data of a vector storage to its initial state after creation
	/// \param[in] configsource configuration string of the model and the database (not a filename!)
	/// \param[in] database database type for the repository
	/// \return true on success, false if an error occurred
	virtual bool resetStorage(
			const std::string& configsrc,
			const DatabaseInterface* database) const=0;

	/// \brief Create a new vector storage client interface
	/// \param[in] configsource configuration string of the model and the database (not a filename!)
	/// \param[in] database database type of the persistent storage where to load the model data from
	/// \return the client interface (with ownership)
	/// \remark The repository refered to by the configuration must exist and must have been built (-> createRepository)
	virtual VectorStorageClientInterface* createClient(
			const std::string& configsource,
			const DatabaseInterface* database) const=0;

	/// \brief Create a dump of the contents of a vector storage
	/// \param[in] configsource Configuration source string describing the storage and the database (not a filename !)
	/// \param[in] database key value store database type used by the vector storage
	/// \param[in] keyprefix prefix for keys to resrict the dump to
	/// \return the object to fetch the dump from
	virtual VectorStorageDumpInterface* createDump(
			const std::string& configsource,
			const DatabaseInterface* database,
			const std::string& keyprefix) const=0;
};

}//namespace
#endif


