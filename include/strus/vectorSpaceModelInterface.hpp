/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for vector space model operations needed for efficient similar vector retrieval in the strus storage
#ifndef _STRUS_VECTOR_SPACE_MODEL_INTERFACE_HPP_INCLUDED
#define _STRUS_VECTOR_SPACE_MODEL_INTERFACE_HPP_INCLUDED
#include <bitset>
#include <vector>

namespace strus {

/// \brief Forward declaration
class VectorSpaceModelBuilderInterface;
/// \brief Forward declaration
class VectorSpaceModelClientInterface;
/// \brief Forward declaration
class VectorSpaceModelDumpInterface;
/// \brief Forward declaration
class DatabaseInterface;


/// \brief Interface for storing an retrieving vectors of floating point numbers representing document features and a model relating them to concept features. The relation is learnt by the model builder in an unsupervised way.
class VectorSpaceModelInterface
{
public:
	/// \brief Destructor
	virtual ~VectorSpaceModelInterface(){}

	/// \brief Create a data repository for the data of a vector space model
	/// \param[in] configsource configuration string of the model and the database (not a filename!)
	/// \param[in] database database type for the repository
	/// \return true on success, false if an error occurred
	/// \remark This method has to be called before any other operation on a vector space model
	virtual bool createRepository(
			const std::string& configsource,
			const DatabaseInterface* database) const=0;

	/// \brief Reset an existing repository for the data of a vector space model to its initial state after creation
	/// \param[in] configsource configuration string of the model and the database (not a filename!)
	/// \param[in] database database type for the repository
	/// \return true on success, false if an error occurred
	virtual bool resetRepository(
			const std::string& configsrc,
			const DatabaseInterface* database) const=0;

	/// \brief Create a new vector space model client interface
	/// \param[in] configsource configuration string of the model and the database (not a filename!)
	/// \param[in] database database type of the persistent storage where to load the model data from
	/// \return the client interface (with ownership)
	/// \remark The repository refered to by the configuration must exist and must have been built (-> createRepository)
	virtual VectorSpaceModelClientInterface* createClient(
			const std::string& configsource,
			const DatabaseInterface* database) const=0;

	/// \brief Create a new vector space model builder
	/// \param[in] configsource configuration string of the model and the database (not a filename!)
	/// \param[in] database database type to use as persistent storage
	/// \return the builder (with ownership)
	/// \remark The repository refered to by the configuration must exist (-> createRepository)
	virtual VectorSpaceModelBuilderInterface* createBuilder(
			const std::string& configsource,
			const DatabaseInterface* database) const=0;

	/// \brief Get the list of builder commands available
	/// \return list of commands for calling 'VectorSpaceModelBuilderInterface::run(const std::string&)'
	/// \remark the process of building a vector space model cannot be unified so easily. Depending on the vsm type you have different commands. This method together with builderCommandDescription(const std::string&) allows the introspection of vsm the builder capabilities.
	virtual std::vector<std::string> builderCommands() const=0;

	/// \brief Get a short description of a builder command
	/// \return the command description text
	virtual std::string builderCommandDescription( const std::string& command) const=0;

	/// \brief Create a dump of a vector space model storage
	/// \param[in] configsource Configuration source string describing the storage and the database (not a filename !)
	/// \param[in] database key value store database type used by the vector space model storage
	/// \param[in] keyprefix prefix for keys to resrict the dump to
	/// \return the object to fetch the dump from
	virtual VectorSpaceModelDumpInterface* createDump(
			const std::string& configsource,
			const DatabaseInterface* database,
			const std::string& keyprefix) const=0;
};

}//namespace
#endif


