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
class QueryEvalInterface;
/// \brief Forward declaration
class QueryProcessorInterface;
/// \brief Forward declaration
class StorageAlterMetaDataTableInterface;


/// \brief Interface providing a mechanism to create complex multi component objects for the storage and the query evaluation in strus.
class StorageObjectBuilderInterface
{
public:
	/// \brief Destructor
	virtual ~StorageObjectBuilderInterface(){}

	/// \brief Get the storage interface
	/// \return the storage interface reference
	virtual const StorageInterface* getStorage() const=0;

	/// \brief Get the database interface as loaded from modules and optionally specified in the configuration string
	/// \param[in] config configuration string (not a filename!) optionally containing an assignment database=<...> that specifies the database to use
	/// \return the database interface reference
	virtual const DatabaseInterface* getDatabase( const std::string& config) const=0;

	/// \brief Get the query processor interface
	/// \return the query processor interface reference
	virtual const QueryProcessorInterface* getQueryProcessor() const=0;

	/// \brief Creates a storage client with its database client out of a configuration string
	/// \param[in] config configuration string (not a filename!) of both storage and database
	/// \return the storage client object (with ownership returned)
	virtual StorageClientInterface* createStorageClient( const std::string& config) const=0;

	/// \brief Creates an interface for altering the storage meta data table with its database client out of a configuration string
	/// \param[in] config configuration string (not a filename!) of both storage and database
	/// \return the interface for altering the storage meta data table (with ownership returned)
	virtual StorageAlterMetaDataTableInterface* createAlterMetaDataTable( const std::string& config) const=0;

	/// \brief Creates a an interface for query evaluation
	/// \return the query evaluation object (with ownership returned)
	virtual QueryEvalInterface* createQueryEval() const=0;
};

}//namespace
#endif

