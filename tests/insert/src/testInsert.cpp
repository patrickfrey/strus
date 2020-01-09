/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "primeFactorCollection.hpp"
#include "strus/reference.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/lib/error.hpp"
#include "strus/lib/database_leveldb.hpp"
#include "strus/lib/storage.hpp"
#include "strus/lib/filelocator.hpp"
#include "strus/base/fileio.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/numstring.hpp"
#include "strus/base/local_ptr.hpp"
#include "strus/base/shared_ptr.hpp"
#include "strus/base/pseudoRandom.hpp"
#include "strus/base/stdint.h"
#include "strus/errorBufferInterface.hpp"
#include "strus/fileLocatorInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/storageTransactionInterface.hpp"
#include "strus/storageMetaDataTableUpdateInterface.hpp"
#include "strus/storageDocumentInterface.hpp"
#include "strus/storageDocumentUpdateInterface.hpp"
#include "strus/storageDumpInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/valueIteratorInterface.hpp"
#include "strus/reference.hpp"
#include "private/errorUtils.hpp"
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <memory>
#include <vector>
#include <map>
#include <set>
#include <limits>
#include <algorithm>

static strus::ErrorBufferInterface* g_errorhnd = 0;
static strus::FileLocatorInterface* g_fileLocator = 0;
static bool g_verbose = false;
static strus::PseudoRandom g_random;

typedef strus::test::PrimeFactorCollection PrimeFactorCollection;
typedef strus::test::PrimeFactorDocumentBuilder PrimeFactorDocumentBuilder;
typedef strus::test::Storage Storage;
typedef strus::test::Feature Feature;

static void buildObserved( PrimeFactorDocumentBuilder& documentBuilder, const std::string& observed)
{
	char const* si = observed.c_str();
	for (;;)
	{
		for (; *si && (unsigned char)*si <= 32; ++si){}
		if (!*si) return;
		char const* dstart = si;
		for (; *si && *si != ':'; ++si){}
		if (!*si) throw std::runtime_error("syntax error in observe pattern, expected ':' separating document id and feature string");
		std::string docid( dstart, si-dstart);
		++si;
		char const* fstart = si;
		for (; *si && (unsigned char)*si > 32; ++si){}
		std::string value( fstart, si-fstart);
		documentBuilder.addObservedTerm( docid, value);
	}
}

static void testInsert( int nofNumbers, int commitSize, const std::string& observed)
{
	Storage storage( g_fileLocator, g_errorhnd);
	storage.open( "path=storage", true);
	storage.defineMetaData( PrimeFactorDocumentBuilder::metadata());

	PrimeFactorDocumentBuilder documentBuilder( nofNumbers, g_verbose, g_errorhnd);
	buildObserved( documentBuilder, observed);

	if (g_verbose) std::cerr << "* inserting/updating documents of generated collection" << std::endl;
	documentBuilder.insertCollection(
		storage.sci.get(), g_random, commitSize, 
		PrimeFactorDocumentBuilder::InsertMode, true/*is last*/);
	if (g_verbose)
	{
		if (nofNumbers)
		{
			std::cerr << strus::string_format( "create collection for numbers from %d to %d with their prime factors as features", PrimeFactorCollection::MinNumber, nofNumbers+PrimeFactorCollection::MinNumber-1) << std::endl;
		}
		int ni = 0, ne = nofNumbers;
		for (; ni != ne; ++ni)
		{
			int df = documentBuilder.primeFactorCollection.frequency( ni+PrimeFactorCollection::MinNumber);
			if (df) std::cerr << strus::string_format( "df %d = %d", ni+PrimeFactorCollection::MinNumber, df) << std::endl;
		}
	}
	if (g_verbose)
	{
		std::cerr << "* dump storage content" << std::endl;
		std::cout << storage.dump() << std::endl;
	}
	// Check inserted documents:
	documentBuilder.checkInsertedDocuments( std::cout, storage.sci.get(), nofNumbers);
	documentBuilder.checkDocumentFrequencies( std::cout, storage.sci.get());
}

static void printUsage()
{
	std::cerr << "usage: testInsert [options] <nofdocs> <commitsize> {<observed>}" << std::endl;
	std::cerr << "description: Inserts a collection of documents representing numbers" << std::endl;
	std::cerr << "             with their prime factors as features. Verify the stored" << std::endl;
	std::cerr << "             data at the end of the test." << std::endl;
	std::cerr << "options:" << std::endl;
	std::cerr << "  -h             :print usage" << std::endl;
	std::cerr << "  -V             :verbose output" << std::endl;
	std::cerr << "  -K             :keep artefacts, do not clean up" << std::endl;
	std::cerr << "<nofdocs>     :number of documents inserted in each (re-)insert cycle" << std::endl;
	std::cerr << "<commitsize>  :number of documents inserted per transaction" << std::endl;
	std::cerr << "<observed>    :list of ':' separated docid:value pairs describing" << std::endl;
	std::cerr << "                  occurrencies printed after every commit." << std::endl;
}


int main( int argc, const char* argv[])
{
	int rt = 0;
	bool do_cleanup = true;
	int argi = 1;
	for (; argc > (int)argi; ++argi)
	{
		if (std::strcmp( argv[argi], "-K") == 0)
		{
			do_cleanup = false;
		}
		else if (std::strcmp( argv[argi], "-V") == 0)
		{
			g_verbose = true;
		}
		else if (std::strcmp( argv[argi], "-h") == 0)
		{
			printUsage();
			return 0;
		}
		else if (std::strcmp( argv[argi], "--") == 0)
		{
			++argi;
			break;
		}
		else if (argv[argi][0] == '-')
		{
			std::cerr << "unknown option " << argv[argi] << std::endl;
			return -1;
		}
		else
		{
			break;
		}
	}
	if (argc == 1)
	{
		printUsage();
		return 0;
	}
	g_errorhnd = strus::createErrorBuffer_standard( stderr, 1, NULL/*debug trace interface*/);
	if (!g_errorhnd) {std::cerr << "FAILED " << "strus::createErrorBuffer_standard" << std::endl; return -1;}
	g_fileLocator = strus::createFileLocator_std( g_errorhnd);
	if (!g_fileLocator) {std::cerr << "FAILED " << "strus::createFileLocator_std" << std::endl; return -1;}

	try
	{
		std::string observed;
		if (argi + 2 > argc)
		{
			printUsage();
			throw std::runtime_error( "too few arguments");
		}
		int nofdocs = strus::numstring_conv::toint( argv[ argi+0], std::strlen(argv[ argi+0]), std::numeric_limits<int>::max());
		int commitsize = strus::numstring_conv::toint( argv[ argi+1], std::strlen(argv[ argi+1]), std::numeric_limits<int>::max());
		int ai = argi+2, ae = argc;
		for (; ai < ae; ++ai)
		{
			observed.push_back( ' ');
			observed.append( argv[ ai]);
		}
		testInsert( nofdocs, commitsize, observed);
		std::cerr << "OK" << std::endl;
	}
	catch (const std::bad_alloc&)
	{
		std::cerr << "out of memory" << std::endl;
		rt = -1;
		goto TESTS_DONE;
	}
	catch (const std::runtime_error& err)
	{
		const char* errmsg = g_errorhnd->fetchError();
		if (errmsg)
		{
			std::cerr << "ERROR " << err.what() << ", " << errmsg << std::endl;
		}
		else
		{
			std::cerr << "ERROR " << err.what() << std::endl;
		}
		rt = -1;
		goto TESTS_DONE;
	}

TESTS_DONE:
	if (do_cleanup)
	{
		Storage::destroy( "path=storage", g_fileLocator, g_errorhnd);
	}
	delete g_fileLocator;
	delete g_errorhnd;
	return rt;
}


