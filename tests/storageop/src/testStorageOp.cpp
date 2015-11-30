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
#include "strus/reference.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/lib/error.hpp"
#include "strus/lib/database_leveldb.hpp"
#include "strus/lib/storage.hpp"
#include "strus/lib/queryproc.hpp"
#include "strus/lib/queryeval.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/storageTransactionInterface.hpp"
#include "strus/storageDocumentInterface.hpp"
#include "private/utils.hpp"
#include <string>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <memory>

static strus::ErrorBufferInterface* g_errorhnd = 0;

class Storage
{
public:
	Storage(){}
	Storage( const Storage& o)
		:dbi(o.dbi),sti(o.sti),sci(o.sci){}
	~Storage(){}

	strus::utils::SharedPtr<strus::DatabaseInterface> dbi;
	strus::utils::SharedPtr<strus::StorageInterface> sti;
	strus::utils::SharedPtr<strus::StorageClientInterface> sci;

	void open( const char* options);
};

class Transaction
{
public:
	explicit Transaction( const Storage& storage_)
		:storage(storage_),tri(storage_.sci->createTransaction()){}
	~Transaction(){}

	bool commit();


private:
	Storage storage;
public:
	strus::utils::SharedPtr<strus::StorageTransactionInterface> tri;
};

void Storage::open( const char* config)
{
	dbi.reset( strus::createDatabase_leveldb( g_errorhnd));
	if (!dbi.get())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
	sti.reset( strus::createStorage( g_errorhnd));
	if (!sti.get())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
	try
	{
		dbi->destroyDatabase( config);
	}
	catch(...){}

	dbi->createDatabase( config);
	std::auto_ptr<strus::DatabaseClientInterface>
		database( dbi->createClient( config));

	sti->createStorage( config, database.get());
	{
		const strus::PeerMessageProcessorInterface* peerMessageProc = 0;
		sci.reset( sti->createClient( "", database.get(), peerMessageProc));
		if (!sci.get())
		{
			throw std::runtime_error( g_errorhnd->fetchError());
		}
		database.release();
	}
}

static void destroyStorage( const char* config)
{
	strus::utils::SharedPtr<strus::DatabaseInterface> dbi;
	dbi.reset( strus::createDatabase_leveldb( g_errorhnd));
	if (!dbi.get())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
	dbi->destroyDatabase( config);
}

bool Transaction::commit()
{
	return tri->commit();
}

static void testDeleteNonExistingDoc()
{
	Storage storage;
	storage.open( "path=storage");
	Transaction transactionInsert( storage);
	std::auto_ptr<strus::StorageDocumentInterface> doc( transactionInsert.tri->createDocument( "ABC"));
	doc->addSearchIndexTerm( "word", "hello", 1);
	doc->addSearchIndexTerm( "word", "world", 2);
	doc->addForwardIndexTerm( "word", "Hello, ", 1);
	doc->addForwardIndexTerm( "word", "world!", 2);
	doc->setAttribute( "title", "Hello World");
	doc->done();
	transactionInsert.commit();

	std::cerr << "Document inserted " << storage.sci->documentNumber( "ABC") << std::endl;

	Transaction transactionDelete1( storage);
	transactionDelete1.tri->deleteDocument( "ABC");
	transactionDelete1.commit();

	std::cerr << "Document deleted " << storage.sci->documentNumber( "ABC") << std::endl;

	Transaction transactionDelete2( storage);
	transactionDelete2.tri->deleteDocument( "ABC");
	transactionDelete2.commit();

	std::cerr << "Document deleted " << storage.sci->documentNumber( "ABC") << std::endl;
	if (g_errorhnd->hasError())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
}

#define RUN_TEST( idx, TestName)\
	try\
	{\
		test ## TestName();\
		std::cerr << "Executing test (" << idx << ") " << #TestName << " [OK]" << std::endl;\
	}\
	catch (const std::runtime_error& err)\
	{\
		std::cerr << "Error in test (" << idx << ") " << #TestName << ": " << err.what() << std::endl;\
		return -1;\
	}\
	catch (const std::bad_alloc& err)\
	{\
		std::cerr << "Out of memory in test (" << idx << ") " << #TestName << std::endl;\
		return -1;\
	}\


int main( int argc, const char* argv[])
{
	bool do_cleanup = true;
	unsigned int ii = 1;
	unsigned int test_index = 0;
	for (; argc > (int)ii; ++ii)
	{
		if (std::strcmp( argv[ii], "-K") == 0)
		{
			do_cleanup = false;
		}
		else if (std::strcmp( argv[ii], "-T") == 0)
		{
			++ii;
			if (argc == (int)ii)
			{
				std::cerr << "option -T expects an argument" << std::endl;
				return -1;
			}
			test_index = atoi( argv[ ii]);
		}
		else if (std::strcmp( argv[ii], "-h") == 0)
		{
			std::cerr << "usage: testStorageOp [options]" << std::endl;
			std::cerr << "options:" << std::endl;
			std::cerr << "  -h      :print usage" << std::endl;
			std::cerr << "  -K      :keep artefacts, do not clean up" << std::endl;
			std::cerr << "  -T <i>  :execute only test with index <i>" << std::endl;
		}
		else if (argv[ii][0] == '-')
		{
			std::cerr << "unknown option " << argv[ii] << std::endl;
			return -1;
		}
		else
		{
			std::cerr << "unexpected argument" << std::endl;
			return -1;
		}
	}
	g_errorhnd = strus::createErrorBuffer_standard( stderr, 1);
	if (!g_errorhnd) return -1;

	unsigned int ti=test_index?test_index:1;
	for (;;++ti)
	{
		switch (ti)
		{
			case 1: RUN_TEST( ti, DeleteNonExistingDoc ) break;
			default: goto TESTS_DONE;
		}
		if (test_index) break;
	}
TESTS_DONE:
	if (do_cleanup)
	{
		destroyStorage( "path=storage");
	}
	return 0;
}


