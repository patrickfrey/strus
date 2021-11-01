/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Builder object for all toplevel interfaces related to the storage. Used by components acting as proxy (calling strus with RPC) or by components that build the storage universe from external components (loading storage objects from dynamically loadable modules)
/// \file storageObjectBuilderInterface.hpp
#ifndef _STRUS_STORAGE_OBJECT_BUILDER_INTERFACE_HPP_INCLUDED
#define _STRUS_STORAGE_OBJECT_BUILDER_INTERFACE_HPP_INCLUDED
#include <string>

namespace strus
{

/// \brief Forward declaration
class DatabaseInterface;
/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class StorageInterface;
/// \brief Forward declaration
class StatisticsStorageInterface;
/// \brief Forward declaration
class QueryEvalInterface;
/// \brief Forward declaration
class QueryProcessorInterface;
/// \brief Forward declaration
class StatisticsProcessorInterface;
/// \brief Forward declaration
class VectorStorageInterface;

/// \brief Interface providing a mechanism to create complex multi component objects for the storage and the query evaluation.
class StorageObjectBuilderInterface
{
public:
	/// \brief Destructor
	virtual ~StorageObjectBuilderInterface(){}

	/// \brief Get the storage interface
	/// \return the storage interface reference
	virtual const StorageInterface* getStorage() const=0;

	/// \brief Get the database interface by name
	/// \param[in] name name of the database interface
	/// \return the database interface reference
	virtual const DatabaseInterface* getDatabase( const std::string& name) const=0;

	/// \brief Get the statistics storage interface
	/// \return the storage interface reference
	virtual const StatisticsStorageInterface* getStatisticsStorage() const=0;

	/// \brief Get the query processor interface
	/// \return the query processor interface reference
	virtual const QueryProcessorInterface* getQueryProcessor() const=0;

	/// \brief Get the statistics processor interface
	/// \param[in] name name of the statistics processor type
	/// \return the statistics processor interface reference
	virtual const StatisticsProcessorInterface* getStatisticsProcessor( const std::string& name) const=0;

	/// \brief Get a vectorspace model for mapping vectors to features by name
	/// \param[in] name name of the vector storage type
	/// \return the vectorspace model interface reference
	virtual const VectorStorageInterface* getVectorStorage( const std::string& name) const=0;

	/// \brief Creates a an interface for query evaluation
	/// \return the query evaluation object (with ownership)
	virtual QueryEvalInterface* createQueryEval() const=0;
};

}//namespace
#endif

