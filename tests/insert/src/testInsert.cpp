/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
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
#include "primeFactorCollection.hpp"
#include <string>
#include <cstring>
#include <cstdlib>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <memory>
#include <vector>
#include <map>
#include <set>
#include <limits>
#include <algorithm>
#include <cmath>

static strus::ErrorBufferInterface* g_errorhnd = 0;
static strus::FileLocatorInterface* g_fileLocator = 0;
static bool g_verbose = false;
static strus::PseudoRandom g_random;
enum {MinRank=0, MaxNofRanks=100, MaxNofClasses=3};

typedef strus::test::PrimeFactorCollection PrimeFactorCollection;
typedef strus::test::PrimeFactorDocumentBuilder PrimeFactorDocumentBuilder;
typedef strus::test::Storage Storage;
typedef strus::test::Feature Feature;

struct StorageDump
{
	struct Element
	{
		int id;
		int pos;

		Element( int id_, int pos_)
			:id(id_),pos(pos_){}
		Element( const Element& o)
			:id(o.id),pos(o.pos){}

		bool operator < (const Element& o) const
		{
			return (pos == o.pos) ? id < o.id : pos < o.pos;
		}
	};
};


static void testInsert( int nofCycles, int nofNumbers, int commitSize)
{
	Storage storage( g_fileLocator, g_errorhnd);
	storage.open( "path=storage", true);
	storage.defineMetaData( PrimeFactorDocumentBuilder::metadata());

	PrimeFactorDocumentBuilder documentBuilder( nofNumbers, g_verbose, g_errorhnd);
	int ci = 0, ce = nofCycles;
	if (nofCycles != 1)
	{
		throw std::runtime_error( "TODO: implement variations of documents (e.g. one feature rendomly eliminated) first before using cycles");
	}
	for (; ci != ce; ++ci)
	{
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
		if (g_verbose) std::cerr << "* inserting documents of generated collection" << std::endl;
		documentBuilder.insert( storage.sci.get(), commitSize);
	}
	if (g_verbose)
	{
		std::cerr << "* dump storage content" << std::endl;
		std::cout << storage.dump() << std::endl;
	}
	// Check inserted documents:
	documentBuilder.checkInsertedDocuments( std::cout, storage.sci.get(), nofNumbers);
}

static void printUsage()
{
	std::cerr << "usage: testInsert [options] <cycles> <nofdocs> <commitsize>" << std::endl;
	std::cerr << "options:" << std::endl;
	std::cerr << "  -h             :print usage" << std::endl;
	std::cerr << "  -V             :verbose output" << std::endl;
	std::cerr << "  -K             :keep artefacts, do not clean up" << std::endl;
	std::cerr << "<cycles>      :number of (re-)insert cycles" << std::endl;
	std::cerr << "<nofdocs>     :number of documents inserted in each (re-)insert cycle" << std::endl;
	std::cerr << "<commitsize>  :number of documents inserted per transaction" << std::endl;
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
		if (argi + 3 > argc) throw std::runtime_error( "too few arguments");
		if (argi + 3 < argc) throw std::runtime_error( "too many arguments");

		int cycles = strus::numstring_conv::toint( argv[ argi+0], std::strlen(argv[ argi+0]), std::numeric_limits<int>::max());
		int nofdocs = strus::numstring_conv::toint( argv[ argi+1], std::strlen(argv[ argi+1]), std::numeric_limits<int>::max());
		int commitsize = strus::numstring_conv::toint( argv[ argi+2], std::strlen(argv[ argi+2]), std::numeric_limits<int>::max());

		testInsert( cycles, nofdocs, commitsize);
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


