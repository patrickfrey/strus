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
#include "strus/valueIteratorInterface.hpp"
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
		const strus::StatisticsProcessorInterface* statisticsMessageProc = 0;
		sci.reset( sti->createClient( "", database.get(), statisticsMessageProc));
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

static void testTermTypeIterator()
{
	Storage storage;
	storage.open( "path=storage");
	Transaction transactionInsert( storage);
	std::auto_ptr<strus::StorageDocumentInterface> doc( transactionInsert.tri->createDocument( "ABC"));
	for (unsigned int ii=0; ii<100; ++ii)
	{
		std::string key( "w");
		key.push_back( '0' + (ii % 10));
		key.push_back( '0' + (ii / 10) % 10);

		doc->addSearchIndexTerm( key, "hello", 1);
		doc->addSearchIndexTerm( key, "world", 2);
	}
	doc->done();
	transactionInsert.commit();

	std::auto_ptr<strus::ValueIteratorInterface> itr( storage.sci->createTermTypeIterator());
	std::vector<std::string> types = itr->fetchValues( 3);
	std::string res;
	while (types.size() > 0)
	{
		std::vector<std::string>::const_iterator ti = types.begin(), te = types.end();
		for (; ti != te; ++ti)
		{
			res.append( *ti);
			res.push_back( ' ');
		}
		types = itr->fetchValues( 3);
	}
	itr->skip( "w61", 3);
	types = itr->fetchValues( 2);
	std::vector<std::string>::const_iterator ti = types.begin(), te = types.end();
	for (; ti != te; ++ti)
	{
		res.append( *ti);
		res.push_back( ' ');
	}
	itr->skip( "w6", 3);
	types = itr->fetchValues( 2);
	ti = types.begin(), te = types.end();
	for (; ti != te; ++ti)
	{
		res.append( *ti);
		res.push_back( ' ');
	}
	itr->skip( "w", 3);
	types = itr->fetchValues( 2);
	ti = types.begin(), te = types.end();
	for (; ti != te; ++ti)
	{
		res.append( *ti);
		res.push_back( ' ');
	}
	if (res != "w00 w01 w02 w03 w04 w05 w06 w07 w08 w09 w10 w11 w12 w13 w14 w15 w16 w17 w18 w19 w20 w21 w22 w23 w24 w25 w26 w27 w28 w29 w30 w31 w32 w33 w34 w35 w36 w37 w38 w39 w40 w41 w42 w43 w44 w45 w46 w47 w48 w49 w50 w51 w52 w53 w54 w55 w56 w57 w58 w59 w60 w61 w62 w63 w64 w65 w66 w67 w68 w69 w70 w71 w72 w73 w74 w75 w76 w77 w78 w79 w80 w81 w82 w83 w84 w85 w86 w87 w88 w89 w90 w91 w92 w93 w94 w95 w96 w97 w98 w99 w61 w62 w60 w61 w00 w01 ")
	{
		throw std::runtime_error("result not as expected");
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
			case 2: RUN_TEST( ti, TermTypeIterator ) break;
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


