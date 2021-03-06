/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Various functions to instantiate strus storage and query evaluation components from configuration programs loaded from source
/// \file programLoader.hpp
#ifndef _STRUS_STORAGE_PROGRAM_LOADER_HPP_INCLUDED
#define _STRUS_STORAGE_PROGRAM_LOADER_HPP_INCLUDED
#include "strus/storage/index.hpp"
#include <string>
#include <vector>
#include <map>

/// \brief strus toplevel namespace
namespace strus {

/// \brief Forward declaration
class FileLocatorInterface;
/// \brief Forward declaration
class QueryProcessorInterface;
/// \brief Forward declaration
class QueryEvalInterface;
/// \brief Forward declaration
class QueryInterface;
/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class VectorStorageClientInterface;
/// \brief Forward declaration
class ErrorBufferInterface;


/// \brief Load a query evaluation program from source
/// \param[in,out] qeval query evaluation interface to instrument
/// \param[in,out] qdescr query descriptors to instrument
/// \param[in] analyzerterms query terms configured in the query analysis configuration
/// \param[in] source source string (not a file name!) to parse
/// \param[in,out] errorhnd buffer for reporting errors (exceptions)
/// \return true on success, false on failure
bool loadQueryEvalProgram(
		QueryEvalInterface& qeval,
		const std::vector<std::string>& analyzerterms,
		const QueryProcessorInterface* qproc,
		const std::string& source,
		ErrorBufferInterface* errorhnd);

/// \brief Load some meta data assignments for a storage from a stream
/// \param[in,out] storage the storage to instrument
/// \param[in] metadataName name of the meta data field to assign
/// \param[in] attributemapref map that maps the update key to a list of document numbers to update (NULL, if the docid or docno is the key)
/// \param[in] file the file to read from
/// \param[in] commitSize number of documents to update until an implicit commit is called (0 => no implicit commit)
/// \param[in,out] errorhnd buffer for reporting errors (exceptions)
/// \return the number of documents (non distinct) updated
int loadDocumentMetaDataAssignments(
		StorageClientInterface& storage,
		const std::string& metadataName,
		const std::multimap<std::string,strus::Index>* attributemapref,
		const std::string& file,
		int commitSize,
		ErrorBufferInterface* errorhnd);

/// \brief Load some attribute assignments for a storage from a stream
/// \param[in,out] storage the storage to instrument
/// \param[in] attributeName name of the attribute to assign
/// \param[in] attributemapref map that maps the update key to a list of document numbers to update (NULL, if the docid or docno is the key)
/// \param[in] file the file to read from
/// \param[in] commitSize number of documents to update until an implicit commit is called (0 => no implicit commit)
/// \param[in,out] errorhnd buffer for reporting errors (exceptions)
/// \return the number of documents (non distinct) updated
int loadDocumentAttributeAssignments(
		StorageClientInterface& storage,
		const std::string& attributeName,
		const std::multimap<std::string,strus::Index>* attributemapref,
		const std::string& file,
		int commitSize,
		ErrorBufferInterface* errorhnd);

/// \brief Load some user rights assignments for a storage from a stream
/// \param[in,out] storage the storage to instrument
/// \param[in] attributemapref map that maps the update key to a list of document numbers to update (NULL, if the docid or docno is the key)
/// \param[in] file the file to read from
/// \param[in] commitSize number of documents to update until an implicit commit is called (0 => no implicit commit)
/// \param[in,out] errorhnd buffer for reporting errors (exceptions)
/// \return the number of documents (non distinct) updated
int loadDocumentUserRightsAssignments(
		StorageClientInterface& storage,
		const std::multimap<std::string,strus::Index>* attributemapref,
		const std::string& file,
		int commitSize,
		ErrorBufferInterface* errorhnd);

/// \brief Adds the feature definitions in the file with path vectorfile to a vector storage
/// \param[in] vstorage vector storage object where to add the loaded vectors to
/// \param[in] vectorfile Path of the file to parse, either a google binary vector file format or text
/// \param[in] networkOrder true, if the vector elements are stored in platform independent network order (hton).
/// \param[in] typeValueSeparator character seperating type and value for typed items, 0 if untyped
/// \param[in] commitSize number of vectors to insert/update until an implicit commit is called (0 => no implicit commit)
/// \param[in,out] errorhnd buffer for reporting errors (exceptions)
/// \return true on success
bool loadVectorStorageVectors( 
		VectorStorageClientInterface* vstorage,
		const std::string& vectorfile,
		bool networkOrder,
		char typeValueSeparator,
		int commitSize,
		ErrorBufferInterface* errorhnd);

}//namespace
#endif

