/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Various functions to instantiate strus storage and query evaluation components from configuration programs loaded from source
/// \file libstrus_storage_prgload_std.cpp
#include "strus/lib/storage_prgload_std.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/base/dll_tags.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "programLoader.hpp"
#include <string>
#include <vector>
#include <map>

static bool g_intl_initialized = false;

using namespace strus;

DLL_PUBLIC bool strus::load_queryeval_program(
		QueryEvalInterface& qeval,
		const std::vector<std::string>& analyzerterms,
		const QueryProcessorInterface* qproc,
		const std::string& source,
		ErrorBufferInterface* errorhnd)
{
	try
	{
		if (!g_intl_initialized)
		{
			strus::initMessageTextDomain();
			g_intl_initialized = true;
		}
		return loadQueryEvalProgram( qeval, analyzerterms, qproc, source, errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error loading query eval program: %s"), *errorhnd, false);
}

DLL_PUBLIC int strus::load_metadata_assignments(
		StorageClientInterface& storage,
		const std::string& metadataName,
		const std::multimap<std::string,strus::Index>* attributemapref,
		const std::string& file,
		int commitsize,
		ErrorBufferInterface* errorhnd)
{
	try
	{
		if (!g_intl_initialized)
		{
			strus::initMessageTextDomain();
			g_intl_initialized = true;
		}
		return loadDocumentMetaDataAssignments( storage, metadataName, attributemapref, file, commitsize, errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error loading metadata assignments: %s"), *errorhnd, -1);
}

DLL_PUBLIC int strus::load_attribute_assignments(
		StorageClientInterface& storage,
		const std::string& attributeName,
		const std::multimap<std::string,strus::Index>* attributemapref,
		const std::string& file,
		int commitsize,
		ErrorBufferInterface* errorhnd)
{
	try
	{
		if (!g_intl_initialized)
		{
			strus::initMessageTextDomain();
			g_intl_initialized = true;
		}
		return loadDocumentAttributeAssignments( storage, attributeName, attributemapref, file, commitsize, errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error loading attribute assignments: %s"), *errorhnd, -1);
}

DLL_PUBLIC int strus::load_user_assignments(
		StorageClientInterface& storage,
		const std::multimap<std::string,strus::Index>* attributemapref,
		const std::string& file,
		int commitsize,
		ErrorBufferInterface* errorhnd)
{
	try
	{
		if (!g_intl_initialized)
		{
			strus::initMessageTextDomain();
			g_intl_initialized = true;
		}
		return loadDocumentUserRightsAssignments( storage, attributemapref, file, commitsize, errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error loading user right assignments: %s"), *errorhnd, -1);
}

DLL_PUBLIC bool strus::load_vectors( 
		VectorStorageClientInterface* vstorage,
		const std::string& vectorfile,
		bool networkOrder,
		char typeValueSeparator,
		ErrorBufferInterface* errorhnd)
{
	try
	{
		if (!g_intl_initialized)
		{
			strus::initMessageTextDomain();
			g_intl_initialized = true;
		}
		return loadVectorStorageVectors( vstorage, vectorfile, networkOrder, typeValueSeparator, errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error loading vectors: %s"), *errorhnd, -1);
}


