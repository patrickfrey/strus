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
class VectorSpaceModelInstanceInterface;
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
			const DatabaseInterface* database)=0;

	/// \brief Reset an existing repository for the data of a vector space model to its initial state after creation
	/// \param[in] configsource configuration string of the model and the database (not a filename!)
	/// \param[in] database database type for the repository
	/// \return true on success, false if an error occurred
	virtual bool resetRepository(
			const std::string& configsrc,
			const DatabaseInterface* database)=0;

	/// \brief Create a new vector space model instance
	/// \param[in] configsource configuration string of the model and the database (not a filename!)
	/// \param[in] database database type of the persistent storage where to load the model data from
	/// \return the instance (with ownership)
	/// \remark The repository refered to by the configuration must exist and must have been built (-> createRepository)
	virtual VectorSpaceModelInstanceInterface* createInstance(
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
};

}//namespace
#endif


