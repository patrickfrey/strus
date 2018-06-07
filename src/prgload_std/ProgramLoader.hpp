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
#include <string>
#include <vector>

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

/// \brief Load a query from source (query language)
/// \param[in,out] query query interface to instrument
/// \param[in] analyzer program for analyzing text segments in the query
/// \param[in] qproc query processor interface for info about objects loaded
/// \param[in] source source string (not a file name!) to parse
/// \param[in] analyzerfields query fields configured in the query analysis configuration
/// \param[in,out] errorhnd buffer for reporting errors (exceptions)
/// \return true on success, false on failure
bool loadQuery(
		QueryInterface& query,
		const QueryAnalyzerInstanceInterface* analyzer,
		const QueryProcessorInterface* qproc,
		const std::string& source,
		const std::vector<std::string>& analyzerfields,
		ErrorBufferInterface* errorhnd);

/// \brief Scan a source for the next program segment in a source that contains multiple programs.
///		The programs are separated by "\r\n.\r\n" or "\n.\n".
///		No escaping of this sequence possible.
/// \param[out] segment the program segment scanned
/// \param[in] itr scanning iterator on a source containing one or multiple programs
/// \param[in] end end iterator of the source to scan
/// \param[in,out] errorhnd buffer for reporting errors (exceptions)
/// \return true, if there was a segment left to scan
/// \note This function is mainly used for loading test programs
/// \remark The scanner skips whitespaces at the start of each program segment and returns initial end of line that belongs to the separator. So whitespaces should have no meaning in the languages of the programs loaded this way.
bool scanNextProgram(
		std::string& segment,
		std::string::const_iterator& itr,
		const std::string::const_iterator& end,
		ErrorBufferInterface* errorhnd);

/// \brief Load some meta data assignments for a storage from a stream
/// \param[in,out] storage the storage to instrument
/// \param[in] metadataName name of the meta data field to assign
/// \param[in] attributemapref map that maps the update key to a list of document numbers to update (NULL, if the docid or docno is the key)
/// \param[in] file the file to read from
/// \param[in] commitsize number of documents to update until an implicit commit is called (0 => no implicit commit)
/// \param[in,out] errorhnd buffer for reporting errors (exceptions)
/// \return the number of documents (non distinct) updated
unsigned int loadDocumentMetaDataAssignments(
		StorageClientInterface& storage,
		const std::string& metadataName,
		const std::multimap<std::string,strus::Index>* attributemapref,
		const std::string& file,
		unsigned int commitsize,
		ErrorBufferInterface* errorhnd);

/// \brief Load some attribute assignments for a storage from a stream
/// \param[in,out] storage the storage to instrument
/// \param[in] attributeName name of the attribute to assign
/// \param[in] attributemapref map that maps the update key to a list of document numbers to update (NULL, if the docid or docno is the key)
/// \param[in] file the file to read from
/// \param[in] commitsize number of documents to update until an implicit commit is called (0 => no implicit commit)
/// \param[in,out] errorhnd buffer for reporting errors (exceptions)
/// \return the number of documents (non distinct) updated
unsigned int loadDocumentAttributeAssignments(
		StorageClientInterface& storage,
		const std::string& attributeName,
		const std::multimap<std::string,strus::Index>* attributemapref,
		const std::string& file,
		unsigned int commitsize,
		ErrorBufferInterface* errorhnd);

/// \brief Load some user rights assignments for a storage from a stream
/// \param[in,out] storage the storage to instrument
/// \param[in] attributemapref map that maps the update key to a list of document numbers to update (NULL, if the docid or docno is the key)
/// \param[in] file the file to read from
/// \param[in] commitsize number of documents to update until an implicit commit is called (0 => no implicit commit)
/// \param[in,out] errorhnd buffer for reporting errors (exceptions)
/// \return the number of documents (non distinct) updated
unsigned int loadDocumentUserRightsAssignments(
		StorageClientInterface& storage,
		const std::multimap<std::string,strus::Index>* attributemapref,
		const std::string& file,
		unsigned int commitsize,
		ErrorBufferInterface* errorhnd);

/// \brief Adds the feature definitions in the file with path vectorfile to a vector storage
/// \param[in] vstorage vector storage object where to add the loaded vectors to
/// \param[in] vectorfile Path of the file to parse, either a google binary vector file format or text
/// \param[in] networkOrder true, if the vector elements are stored in platform independent network order (hton).
/// \param[in,out] errorhnd buffer for reporting errors (exceptions)
/// \return true on success
bool loadVectorStorageVectors( 
		VectorStorageClientInterface* vstorage,
		const std::string& vectorfile,
		bool networkOrder,
		ErrorBufferInterface* errorhnd);

}//namespace
#endif

