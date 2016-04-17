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
#include "strus/lib/queryproc.hpp"
#include "strus/lib/queryeval.hpp"
#include "strus/base/fileio.hpp"
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
#include "private/errorUtils.hpp"
#include <string>
#include <cstring>
#include <stdio.h>
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


void Storage::open( const char* config)
{
	dbi.reset( strus::createDatabase_leveldb( g_errorhnd));
	if (!dbi.get())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
	sti.reset( strus::createStorage( g_errorhnd));
	if (!sti.get() || g_errorhnd->hasError())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
	(void)dbi->destroyDatabase( config);
	(void)g_errorhnd->fetchError();

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

static void testDeleteNonExistingDoc()
{
	Storage storage;
	storage.open( "path=storage");
	std::auto_ptr<strus::StorageTransactionInterface> transactionInsert( storage.sci->createTransaction());
	std::auto_ptr<strus::StorageDocumentInterface> doc( transactionInsert->createDocument( "ABC"));
	doc->addSearchIndexTerm( "word", "hello", 1);
	doc->addSearchIndexTerm( "word", "world", 2);
	doc->addForwardIndexTerm( "word", "Hello, ", 1);
	doc->addForwardIndexTerm( "word", "world!", 2);
	doc->setAttribute( "title", "Hello World");
	doc->done();
	transactionInsert->commit();

	std::cerr << "Document inserted " << storage.sci->documentNumber( "ABC") << std::endl;

	std::auto_ptr<strus::StorageTransactionInterface> transactionDelete1( storage.sci->createTransaction());
	transactionDelete1->deleteDocument( "ABC");
	transactionDelete1->commit();

	std::cerr << "Document deleted " << storage.sci->documentNumber( "ABC") << std::endl;

	std::auto_ptr<strus::StorageTransactionInterface> transactionDelete2( storage.sci->createTransaction());
	transactionDelete2->deleteDocument( "ABC");
	transactionDelete2->commit();

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
	std::auto_ptr<strus::StorageTransactionInterface> transactionInsert( storage.sci->createTransaction());
	std::auto_ptr<strus::StorageDocumentInterface> doc( transactionInsert->createDocument( "ABC"));
	for (unsigned int ii=0; ii<100; ++ii)
	{
		std::string key( "w");
		key.push_back( '0' + (ii % 10));
		key.push_back( '0' + (ii / 10) % 10);

		doc->addSearchIndexTerm( key, "hello", 1);
		doc->addSearchIndexTerm( key, "world", 2);
	}
	doc->done();
	transactionInsert->commit();

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
	if (g_errorhnd->hasError())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
	if (res != "w00 w01 w02 w03 w04 w05 w06 w07 w08 w09 w10 w11 w12 w13 w14 w15 w16 w17 w18 w19 w20 w21 w22 w23 w24 w25 w26 w27 w28 w29 w30 w31 w32 w33 w34 w35 w36 w37 w38 w39 w40 w41 w42 w43 w44 w45 w46 w47 w48 w49 w50 w51 w52 w53 w54 w55 w56 w57 w58 w59 w60 w61 w62 w63 w64 w65 w66 w67 w68 w69 w70 w71 w72 w73 w74 w75 w76 w77 w78 w79 w80 w81 w82 w83 w84 w85 w86 w87 w88 w89 w90 w91 w92 w93 w94 w95 w96 w97 w98 w99 w61 w62 w60 w61 w00 w01 ")
	{
		throw std::runtime_error("result not as expected");
	}
}

struct Feature
{
	enum Kind
	{
		SearchIndex,
		ForwardIndex,
		Attribute,
		MetaData
	};
	Kind kind;
	std::string type;
	std::string value;
	unsigned int pos;

	Feature( Kind kind_, const std::string& type_, const std::string& value_, unsigned int pos_=0)
		:kind(kind_),type(type_),value(value_),pos(pos_){}
	Feature( const Feature& o)
		:kind(o.kind),type(o.type),value(o.value),pos(o.pos){}
};

struct DocumentBuilder
{
	struct Dim
	{
		unsigned int nofDocs;
		unsigned int nofTermTypes;
		unsigned int nofTermValues;
		unsigned int nofDiffTermValues;
		unsigned int nofAttributes;
		unsigned int nofMetaData;
	};

	static std::vector<Feature> create( unsigned int docno, const Dim& dim)
	{
		std::vector<Feature> rt;

		unsigned int ti=0, te=dim.nofTermTypes;
		for (ti=0; ti<te; ++ti)
		{
			char searchtype[ 32];
			snprintf( searchtype, sizeof(searchtype), "q%02u", (ti + 10 * docno) % dim.nofTermTypes);
			char forwardtype[ 32];
			snprintf( forwardtype, sizeof(forwardtype), "r%02u", (ti + 10 * docno) % dim.nofTermTypes);

			unsigned int vi=0, ve=dim.nofTermValues;
			for (vi=0; vi<ve; ++vi)
			{
				if ((vi + docno) % 4 == 0) continue;
				char value[ 32];
				snprintf( value, sizeof(value), "s%02u", (vi + 10 * docno) % dim.nofDiffTermValues);
				rt.push_back( Feature( Feature::SearchIndex, searchtype, value, vi+1));

				snprintf( value, sizeof(value), "f%02u", (vi + 10 * docno) % dim.nofDiffTermValues);
				rt.push_back( Feature( Feature::ForwardIndex, forwardtype, value, vi+1));
			}
		}
		unsigned int ai=0, ae=dim.nofAttributes;
		for (ai=0; ai<ae; ++ai)
		{
			char attributename[ 32];
			snprintf( attributename, sizeof(attributename), "A%02u", ai);
			char attributevalue[ 32];
			snprintf( attributevalue, sizeof(attributevalue), "a%02u", ai);

			rt.push_back( Feature( Feature::Attribute, attributename, attributevalue));
		}
		unsigned int mi=0, me=dim.nofMetaData;
		for (mi=0; mi<me; ++mi)
		{
			char metadataname[ 32];
			snprintf( metadataname, sizeof(metadataname), "M%1u", mi);
			char metadatavalue[ 32];
			snprintf( metadatavalue, sizeof(metadatavalue), "m%1u", mi * 11);
			
			rt.push_back( Feature( Feature::MetaData, metadataname, metadatavalue));
		}
		return rt;
	}
};

static void insertDocument( strus::StorageDocumentInterface* doc, const std::vector<Feature>& featurelist)
{
	std::vector<Feature>::const_iterator fi = featurelist.begin(), fe = featurelist.end();
	for (; fi != fe; ++fi)
	{
		switch (fi->kind)
		{
			case Feature::SearchIndex:
				doc->addSearchIndexTerm( fi->type, fi->value, fi->pos);
				break;
			case Feature::ForwardIndex:
				doc->addForwardIndexTerm( fi->type, fi->value, fi->pos);
				break;
			case Feature::Attribute:
				doc->setAttribute( fi->type, fi->value);
				break;
			case Feature::MetaData:
				doc->setMetaData( fi->type, (unsigned int)atoi(fi->value.c_str()));
				break;
		}
	}
	doc->done();
	
}

static void insertCollection( strus::StorageClientInterface* storage, const DocumentBuilder::Dim& dim)
{
	std::auto_ptr<strus::StorageTransactionInterface> transaction( storage->createTransaction());
	unsigned int di=0, de=dim.nofDocs;
	for (; di != de; ++di)
	{
		char docid[ 32];
		snprintf( docid, sizeof(docid), "D%02u", di);
		std::auto_ptr<strus::StorageDocumentInterface>
			doc( transaction->createDocument( docid));
		if (!doc.get()) throw strus::runtime_error("error creating document to insert");

		std::vector<Feature> feats = DocumentBuilder::create( di, dim);
		insertDocument( doc.get(), feats);
	}
	if (!transaction->commit() || g_errorhnd->hasError())
	{
		throw strus::runtime_error( "transaction failed: %s", g_errorhnd->fetchError());
	}
}

typedef std::pair<std::string,std::string> DfMapKey;
typedef std::map<DfMapKey,strus::Index> DfMap;

static void calculateDocumentDfMap( DfMap& dfmap, const DocumentBuilder::Dim& dim, unsigned int docno)
{
	std::set<DfMapKey> visited;
	std::vector<Feature> feats = DocumentBuilder::create( docno, dim);
	std::vector<Feature>::const_iterator fi = feats.begin(), fe = feats.end();
	for (; fi != fe; ++fi)
	{
		if (fi->kind == Feature::SearchIndex)
		{
			DfMapKey key = DfMapKey( fi->type, fi->value);
			if (visited.find( key) == visited.end())
			{
				visited.insert( key);
				dfmap[ key] += 1;
			}
		}
	}
}

static DfMap calculateCollectionDfMap( const DocumentBuilder::Dim& dim)
{
	DfMap dfmap;
	unsigned int di=0, de=dim.nofDocs;
	for (; di != de; ++di)
	{
		calculateDocumentDfMap( dfmap, dim, di);
	}
	return dfmap;
}

static void testTrivialInsert()
{
	DocumentBuilder::Dim dim;
	dim.nofDocs = 100;
	dim.nofTermTypes = 10;
	dim.nofTermValues = 100;
	dim.nofDiffTermValues = 50;
	dim.nofAttributes = 3;
	dim.nofMetaData = 3;

	Storage storage;
	storage.open( "path=storage; metadata=M0 UINT32, M1 UINT16, M2 UINT8");
	insertCollection( storage.sci.get(), dim);

	unsigned int di=0,de=dim.nofDocs;
	for (; di != de; ++di)
	{
		char docid[ 32];
		snprintf( docid, sizeof(docid), "D%02u", di);
		const char* errlog = "checkindex.log";

		unsigned int ec = strus::writeFile( errlog, "");
		if (ec) throw strus::runtime_error("error opening logfile '%s' (%u)", errlog, ec);

		std::auto_ptr<strus::StorageDocumentInterface>
			doc( storage.sci->createDocumentChecker( docid, errlog));
		if (!doc.get())
		{
			throw std::runtime_error( g_errorhnd->fetchError());
		}

		std::vector<Feature> feats = DocumentBuilder::create( di, dim);
		insertDocument( doc.get(), feats);

		if (g_errorhnd->hasError())
		{
			throw std::runtime_error( g_errorhnd->fetchError());
		}
		std::string errors;
		ec = strus::readFile( errlog, errors);
		if (ec) throw strus::runtime_error("error opening logfile '%s' for reading (%u)", errlog, ec);
		if (errors.size() > 1000)
		{
			errors.resize(1000);
		}
		if (!errors.empty())
		{
			throw strus::runtime_error("error checking insert of %s: %s", docid, errors.c_str());
		}
	}
}

static void testDfCalculation()
{
	DocumentBuilder::Dim dim;
	dim.nofDocs = 100;
	dim.nofTermTypes = 3;
	dim.nofTermValues = 1000;
	dim.nofDiffTermValues = 200;
	dim.nofAttributes = 0;
	dim.nofMetaData = 0;

	Storage storage;
	storage.open( "path=storage; metadata=M0 UINT32, M1 UINT16, M2 UINT8");
	insertCollection( storage.sci.get(), dim);

	DfMap dfmap = calculateCollectionDfMap( dim);
	DfMap::const_iterator xi = dfmap.begin(), xe = dfmap.end();
	for (; xi != xe; ++xi)
	{
		strus::Index df = storage.sci->documentFrequency( xi->first.first, xi->first.second);
		std::cout << "GET FEATURE DF " << xi->first.first << " '" << xi->first.second << "' = " << df << std::endl;
		if (df != xi->second) throw strus::runtime_error("df of feature %s '%s' does not match: %d != %d", xi->first.first.c_str(), xi->first.second.c_str(), (int)df, (int)xi->second);
	}
	{
		std::auto_ptr<strus::StorageTransactionInterface> transaction( storage.sci->createTransaction());
		unsigned int di=0, de=dim.nofDocs;
		for (; di < de; di+=2)
		{
			char docid[ 32];
			snprintf( docid, sizeof(docid), "D%02u", di);
	
			transaction->deleteDocument( docid);
			DfMap doc_dfmap;
			calculateDocumentDfMap( doc_dfmap, dim, di);
			
			xi = doc_dfmap.begin(), xe = doc_dfmap.end();
			for (; xi != xe; ++xi)
			{
				DfMap::iterator mi = dfmap.find( DfMapKey( xi->first.first, xi->first.second));
				if (mi == dfmap.end()) throw strus::runtime_error("feature %s '%s' not found in dfmap", xi->first.first.c_str(), xi->first.second.c_str());
				if (mi->second < xi->second) throw strus::runtime_error("df of feature %s '%s' got negative after deletion", xi->first.first.c_str(), xi->first.second.c_str());
				mi->second -= xi->second;
			}
		}
		transaction->commit();
	}
	{
		std::auto_ptr<strus::StorageTransactionInterface> transaction( storage.sci->createTransaction());
		xi = dfmap.begin(), xe = dfmap.end();
		for (; xi != xe; ++xi)
		{
			strus::Index df = storage.sci->documentFrequency( xi->first.first, xi->first.second);
			if (df != xi->second) throw strus::runtime_error("df of feature %s '%s' does not match after deletion of documents: %d != %d", xi->first.first.c_str(), xi->first.second.c_str(), (int)df, (int)xi->second);
		}
		unsigned int di=0, de=dim.nofDocs;
		for (di=1; di < de; di+=2)
		{
			char docid[ 32];
			snprintf( docid, sizeof(docid), "D%02u", di);
	
			transaction->deleteDocument( docid);
		}
		transaction->commit();
	}
	if (g_errorhnd->hasError())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
	xi = dfmap.begin(), xe = dfmap.end();
	for (; xi != xe; ++xi)
	{
		strus::Index df = storage.sci->documentFrequency( xi->first.first, xi->first.second);
		if (df != 0) throw strus::runtime_error("df of feature %s '%s' got not null after deletion of all documents: %d", xi->first.first.c_str(), xi->first.second.c_str(), (int)df);
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
			case 1: RUN_TEST( ti, DfCalculation ) break;
			case 2: RUN_TEST( ti, DeleteNonExistingDoc ) break;
			case 3: RUN_TEST( ti, TermTypeIterator ) break;
			case 4: RUN_TEST( ti, TrivialInsert ) break;
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


