/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "strus/reference.hpp"
#include "strus/lib/error.hpp"
#include "strus/lib/filelocator.hpp"
#include "strus/lib/database_leveldb.hpp"
#include "strus/lib/storage.hpp"
#include "strus/lib/queryproc.hpp"
#include "strus/lib/queryeval.hpp"
#include "strus/base/fileio.hpp"
#include "strus/base/local_ptr.hpp"
#include "strus/base/string_format.hpp"
#include "strus/fileLocatorInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/debugTraceInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/queryEvalInterface.hpp"
#include "strus/queryInterface.hpp"
#include "strus/queryResult.hpp"
#include "strus/summaryElement.hpp"
#include "strus/storageInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/storageTransactionInterface.hpp"
#include "strus/storageMetaDataTableUpdateInterface.hpp"
#include "strus/storageDumpInterface.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/metaDataRestrictionInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/summarizerFunctionInstanceInterface.hpp"
#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/index.hpp"
#include "private/errorUtils.hpp"
#include "strus/base/shared_ptr.hpp"
#include <string>
#include <cstring>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <memory>
#include <algorithm>

static strus::DebugTraceInterface* g_dbgtrace = 0;
static strus::ErrorBufferInterface* g_errorhnd = 0;
static strus::FileLocatorInterface* g_fileLocator = 0;
static bool g_verbose = false;

class Storage
{
public:
	Storage(){}
	Storage( const Storage& o)
		:dbi(o.dbi),sti(o.sti),sci(o.sci){}
	~Storage(){}

	strus::shared_ptr<strus::DatabaseInterface> dbi;
	strus::shared_ptr<strus::StorageInterface> sti;
	strus::shared_ptr<strus::StorageClientInterface> sci;

	struct MetaDataDef
	{
		const char* key;
		const char* type;
	};
	void defineMetaData( MetaDataDef const* deflist);
	void open( const char* options);
};


void Storage::open( const char* config)
{
	dbi.reset( strus::createDatabaseType_leveldb( g_fileLocator, g_errorhnd));
	if (!dbi.get())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
	sti.reset( strus::createStorageType_std( g_fileLocator, g_errorhnd));
	if (!sti.get() || g_errorhnd->hasError())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
	(void)dbi->destroyDatabase( config);
	(void)g_errorhnd->fetchError();

	sti->createStorage( config, dbi.get());
	{
		const strus::StatisticsProcessorInterface* statisticsMessageProc = 0;
		sci.reset( sti->createClient( config, dbi.get(), statisticsMessageProc));
		if (!sci.get())
		{
			throw std::runtime_error( g_errorhnd->fetchError());
		}
	}
}

void Storage::defineMetaData( MetaDataDef const* deflist)
{
	strus::Reference<strus::StorageTransactionInterface> transaction( sci->createTransaction());
	if (!transaction.get()) throw strus::runtime_error( "failed to create transaction: %s", g_errorhnd->fetchError());
	strus::Reference<strus::StorageMetaDataTableUpdateInterface> update( transaction->createMetaDataTableUpdate());
	if (!update.get()) throw strus::runtime_error( "failed to create structure for declaring meta data: %s", g_errorhnd->fetchError());
	
	int di = 0;
	for (; deflist[di].key; ++di)
	{
		update->addElement( deflist[di].key, deflist[di].type);
	}
	update->done();
	if (!transaction->commit()) throw strus::runtime_error( "failed to commit meta data structure definition: %s", g_errorhnd->fetchError());
}

static void destroyStorage( const char* config)
{
	strus::shared_ptr<strus::DatabaseInterface> dbi;
	dbi.reset( strus::createDatabaseType_leveldb( g_fileLocator, g_errorhnd));
	if (!dbi.get())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
	dbi->destroyDatabase( config);
}


class QueryEvaluationEnv
{
public:
	Storage storage;
	strus::local_ptr<strus::QueryEvalInterface> qeval;
	strus::local_ptr<strus::QueryInterface> query;

	explicit QueryEvaluationEnv( const strus::QueryProcessorInterface* qpi)
	{
		static const unsigned int primes[5] = {2,3,5,7,0};
		storage.open( "path=storage");
		const Storage::MetaDataDef metadata[] = {{"docno", "UINT16"},{0,0}};
		storage.defineMetaData( metadata);
		strus::local_ptr<strus::StorageTransactionInterface> transactionInsert( storage.sci->createTransaction());
		enum {NofDocs=10};
		unsigned int di=0,de=NofDocs;
		for (; di < de; ++di)
		{
			char docid[10];
			snprintf( docid, sizeof(docid), "DOC%u", di);

			if (g_verbose) std::cerr << "insert document " << docid << std::endl;

			strus::local_ptr<strus::StorageDocumentInterface> doc( transactionInsert->createDocument( docid));
			doc->addSearchIndexTerm( "word", "hello", 1);
			doc->addSearchIndexTerm( "word", "world", 2);

			if (g_verbose) std::cerr << "document " << docid << " add search index term \"word\" \"hello\"" << std::endl;
			if (g_verbose) std::cerr << "document " << docid << " add search index term \"word\" \"world\"" << std::endl;

			unsigned int xi = di;
			unsigned int fpos = 10;
			for (unsigned int pidx=0; primes[pidx]; ++pidx)
			{
				char primbuf[10];
				snprintf( primbuf, sizeof(primbuf), "%u", primes[pidx]);
				if (xi) while (xi % primes[pidx] == 0)
				{
					doc->addSearchIndexTerm( "prim", primbuf, ++fpos);

					if (g_verbose) std::cerr << "document " << docid << " add search index term \"prim\" \"" << primbuf << "\" at pos " << fpos << std::endl;

					xi /= primes[pidx];
				}
			}
			doc->addSearchIndexTerm( "word", docid, 3);
			doc->setMetaData( "docno", (strus::NumericVariant::IntType)di);
			doc->setAttribute( "docid", docid);

			if (g_verbose) std::cerr << "document " << docid << " add search index term \"word\" \"" << docid << "\"" << std::endl;
			if (g_verbose) std::cerr << "document " << docid << " add attribute \"docid\" \"" << docid << "\"" << std::endl;

			doc->done();
		}
		if (!transactionInsert->commit()) throw std::runtime_error("failed to insert test documents");
		if (g_verbose)
		{
			strus::Reference<strus::StorageDumpInterface> dump( storage.sci->createDump(""));
			if (!dump.get()) throw std::runtime_error( g_errorhnd->fetchError());
			const char* chunk;
			std::size_t chunksize;

			std::cerr << "storage dump:" << std::endl;
			while (dump->nextChunk( chunk, chunksize))
			{
				std::cerr << std::string( chunk, chunksize) << std::endl;
			}
			std::cerr << std::endl;
		}

		qeval.reset( strus::createQueryEval( g_errorhnd));
		if (!qeval.get()) throw std::runtime_error("failed to create query eval");
		const strus::SummarizerFunctionInterface* summarizer = qpi->getSummarizerFunction( "attribute");
		if (!summarizer) throw std::runtime_error("failed to get summarizer");
		strus::SummarizerFunctionInstanceInterface* summarizerInstance = summarizer->createInstance( qpi);
		if (!summarizerInstance) throw std::runtime_error("failed to create summarizer instance");
		summarizerInstance->addStringParameter( "name", "docid");
		qeval->addSummarizerFunction( "docid", summarizerInstance, std::vector<strus::QueryEvalInterface::FeatureParameter>());

		qeval->addSelectionFeature( "sel");
		qeval->addRestrictionFeature( "res");
		qeval->addExclusionFeature( "exc");
	
		const strus::WeightingFunctionInterface* weighting = qpi->getWeightingFunction( "frequency");
		if (!weighting) throw std::runtime_error("failed to get weighting function");
		strus::WeightingFunctionInstanceInterface* weightingInstance = weighting->createInstance( qpi);
		if (!weightingInstance) throw std::runtime_error("failed to create weighting function instance");
		std::vector<strus::QueryEvalInterface::FeatureParameter> weightingFeatures;
		weightingFeatures.push_back( strus::QueryEvalInterface::FeatureParameter( "match", "qry"));
		qeval->addWeightingFunction( weightingInstance, weightingFeatures);

		if (g_errorhnd->hasError())
		{
			throw std::runtime_error("failed to create query evaluation environment");
		}
		query.reset( qeval->createQuery( storage.sci.get()));
	}
};

static void printQueryResult( const strus::QueryResult& result)
{
	std::vector<strus::ResultDocument>::const_iterator ri = result.ranks().begin(), re = result.ranks().end(); 
	for (int ridx=0; ri != re; ++ri,++ridx)
	{
		std::ostringstream resdescr;
		std::vector<strus::SummaryElement>::const_iterator si = ri->summaryElements().begin(), se = ri->summaryElements().end();
		for (int sidx=0; si != se; ++si,++sidx)
		{
			if (sidx) resdescr << ", ";
			resdescr << si->name() << "=" << si->value();
		}
		std::cerr << "[" << ridx << "] " << ri->weight() << ": " << resdescr.str() << std::endl;
	}
}

static std::string getQueryResultMembersString( const strus::QueryResult& result)
{
	std::string rt;
	std::vector<std::string> resbuf;
	std::vector<strus::ResultDocument>::const_iterator ri = result.ranks().begin(), re = result.ranks().end(); 
	for (int ridx=0; ri != re; ++ri,++ridx)
	{
		std::vector<strus::SummaryElement>::const_iterator si = ri->summaryElements().begin(), se = ri->summaryElements().end();
		for (int sidx=0; si != se; ++si,++sidx)
		{
			if (si->name() == "docid")
			{
				resbuf.push_back( std::string( si->value().c_str()+3));
			}
		}
	}
	std::sort( resbuf.begin(), resbuf.end());
	std::vector<std::string>::const_iterator ti = resbuf.begin(), te = resbuf.end();
	for (int tidx=0; ti != te; ++ti,++tidx)
	{
		if (tidx) rt.push_back( ',');
		rt.append( *ti);
	}
	return rt;
}

static void testPlainSingleTermQuery( const strus::QueryProcessorInterface* qpi)
{
	QueryEvaluationEnv queryenv( qpi);
	strus::QueryInterface* query = queryenv.query.get();

	query->pushTerm( "word", "hello", 1);
	query->defineFeature( "qry");
	query->pushTerm( "word", "hello", 1);
	query->defineFeature( "sel");

	strus::QueryResult result = query->evaluate();

	if (g_verbose) std::cerr << "result testPlainSingleTermQuery:" << std::endl;
	if (g_verbose) printQueryResult( result);

	std::string res = getQueryResultMembersString( result);
	std::string exp = "0,1,2,3,4,5,6,7,8,9";

	if (g_verbose) std::cerr << "packed result: (" << res << ")" << std::endl;
	if (g_verbose) std::cerr << "expected: (" << exp << ")" << std::endl;

	if (res != exp)
	{
		throw std::runtime_error("query result not as expected");
	}
}

static void testSingleTermQueryWithExclusion( const strus::QueryProcessorInterface* qpi)
{
	QueryEvaluationEnv queryenv( qpi);
	strus::QueryInterface* query = queryenv.query.get();

	query->pushTerm( "word", "hello", 1);
	query->defineFeature( "qry");
	query->pushTerm( "word", "hello", 1);
	query->defineFeature( "sel");
	query->pushTerm( "prim", "2", 1);
	query->defineFeature( "exc");
	query->pushTerm( "prim", "3", 1);
	query->defineFeature( "exc");

	strus::QueryResult result = query->evaluate();

	if (g_verbose) std::cerr << "result testSingleTermQueryWithExclusion:" << std::endl;
	if (g_verbose) printQueryResult( result);

	std::string res = getQueryResultMembersString( result);
	std::string exp = "0,1,5,7";

	if (g_verbose) std::cerr << "packed result: (" << res << ")" << std::endl;
	if (g_verbose) std::cerr << "expected: (" << exp << ")" << std::endl;

	if (res != exp)
	{
		throw std::runtime_error("query result not as expected");
	}
}

static void testSingleTermQueryWithRestriction( const strus::QueryProcessorInterface* qpi)
{
	QueryEvaluationEnv queryenv( qpi);
	strus::QueryInterface* query = queryenv.query.get();
	const strus::PostingJoinOperatorInterface* operation_OR = qpi->getPostingJoinOperator( "union");
	if (!operation_OR) throw std::runtime_error("operation 'union' is not defined");

	query->pushTerm( "word", "hello", 1);
	query->defineFeature( "qry");
	query->pushTerm( "word", "hello", 1);
	query->defineFeature( "sel");
	query->pushTerm( "prim", "2", 1);
	query->pushTerm( "prim", "3", 1);
	query->pushExpression( operation_OR, 2, 0, 0);
	query->defineFeature( "res");

	strus::QueryResult result = query->evaluate();

	if (g_verbose) std::cerr << "result testSingleTermQueryWithRestriction:" << std::endl;
	if (g_verbose) printQueryResult( result);

	std::string res = getQueryResultMembersString( result);
	std::string exp = "2,3,4,6,8,9";

	if (g_verbose) std::cerr << "packed result: (" << res << ")" << std::endl;
	if (g_verbose) std::cerr << "expected: (" << exp << ")" << std::endl;

	if (res != exp)
	{
		throw std::runtime_error("query result not as expected");
	}
}


static void testSingleTermQueryWithRestrictionInclMetadata( const strus::QueryProcessorInterface* qpi)
{
	QueryEvaluationEnv queryenv( qpi);
	strus::QueryInterface* query = queryenv.query.get();
	const strus::PostingJoinOperatorInterface* operation_OR = qpi->getPostingJoinOperator( "union");
	if (!operation_OR) throw std::runtime_error("operation 'union' is not defined");

	query->pushTerm( "word", "hello", 1);
	query->defineFeature( "qry");
	query->pushTerm( "word", "hello", 1);
	query->defineFeature( "sel");
	query->pushTerm( "prim", "2", 1);
	query->pushTerm( "prim", "3", 1);
	query->pushExpression( operation_OR, 2, 0, 0);
	query->defineFeature( "res");
	query->addMetaDataRestrictionCondition( strus::MetaDataRestrictionInterface::CompareLess, "docno", (strus::NumericVariant::IntType)9, true);
	query->addMetaDataRestrictionCondition( strus::MetaDataRestrictionInterface::CompareGreater, "docno", (strus::NumericVariant::IntType)2, true);
	query->addMetaDataRestrictionCondition( strus::MetaDataRestrictionInterface::CompareEqual, "docno", (strus::NumericVariant::IntType)3, true);
	query->addMetaDataRestrictionCondition( strus::MetaDataRestrictionInterface::CompareEqual, "docno", (strus::NumericVariant::IntType)4, false);
	query->addMetaDataRestrictionCondition( strus::MetaDataRestrictionInterface::CompareEqual, "docno", (strus::NumericVariant::IntType)5, false);
	query->addMetaDataRestrictionCondition( strus::MetaDataRestrictionInterface::CompareEqual, "docno", (strus::NumericVariant::IntType)6, false);

	strus::QueryResult result = query->evaluate();

	if (g_verbose) std::cerr << "result testSingleTermQueryWithRestrictionInclMetadata:" << std::endl;
	if (g_verbose) printQueryResult( result);

	std::string res = getQueryResultMembersString( result);
	std::string exp = "3,4,6";

	if (g_verbose) std::cerr << "packed result: (" << res << ")" << std::endl;
	if (g_verbose) std::cerr << "expected: (" << exp << ")" << std::endl;

	if (res != exp)
	{
		throw std::runtime_error("query result not as expected");
	}
}


static void testSingleTermQueryWithSelectionAndRestriction( const strus::QueryProcessorInterface* qpi)
{
	QueryEvaluationEnv queryenv( qpi);
	strus::QueryInterface* query = queryenv.query.get();
	const strus::PostingJoinOperatorInterface* operation_OR = qpi->getPostingJoinOperator( "union");
	if (!operation_OR) throw std::runtime_error("operation 'union' is not defined");

	query->pushTerm( "word", "hello", 1);
	query->defineFeature( "qry");
	query->pushTerm( "prim", "2", 1);
	query->defineFeature( "sel");
	query->pushTerm( "prim", "3", 1);
	query->defineFeature( "res");

	strus::QueryResult result = query->evaluate();

	if (g_verbose) std::cerr << "result testSingleTermQueryWithRestriction:" << std::endl;
	if (g_verbose) printQueryResult( result);

	std::string res = getQueryResultMembersString( result);
	std::string exp = "6";

	if (g_verbose) std::cerr << "packed result: (" << res << ")" << std::endl;
	if (g_verbose) std::cerr << "expected: (" << exp << ")" << std::endl;

	if (res != exp)
	{
		throw std::runtime_error("query result not as expected");
	}
}


#define RUN_TEST( idx, TestName, qpi, rt)\
	try\
	{\
		test ## TestName( qpi);\
		std::cerr << "Executing test (" << idx << ") " << #TestName << " [OK]" << std::endl;\
	}\
	catch (const std::runtime_error& err)\
	{\
		std::cerr << "error in test (" << idx << ") " << #TestName << ": " << err.what() << std::endl;\
		rt = -1;\
	}\
	catch (const std::bad_alloc& err)\
	{\
		std::cerr << "Out of memory in test (" << idx << ") " << #TestName << std::endl;\
		rt = -1;\
	}\


int main( int argc, const char* argv[])
{
	int rt = 0;
	bool do_cleanup = true;
	try
	{
		g_dbgtrace = strus::createDebugTrace_standard( 2/*maxNofThreads*/);
		if (!g_dbgtrace) {std::cerr << "FAILED " << "strus::createDebugTrace_standard" << std::endl; return -1;}

		unsigned int ii = 1;
		unsigned int test_index = 0;
		for (; argc > (int)ii; ++ii)
		{
			if (std::strcmp( argv[ii], "-T") == 0)
			{
				++ii;
				if (argc == (int)ii)
				{
					std::cerr << "option -T expects an argument" << std::endl;
					return -1;
				}
				test_index = atoi( argv[ ii]);
			}
			else if (std::strcmp( argv[ii], "-V") == 0)
			{
				g_verbose = true;
			}
			else if (std::strcmp( argv[ii], "-G") == 0)
			{
				++ii;
				if (argc == (int)ii)
				{
					std::cerr << "option -G expects an argument" << std::endl;
					return -1;
				}
				g_dbgtrace->enable( argv[ ii]);
			}
			else if (std::strcmp( argv[ii], "-h") == 0)
			{
				std::cerr << "usage: testQuery [options]" << std::endl;
				std::cerr << "options:" << std::endl;
				std::cerr << "  -h      :print usage" << std::endl;
				std::cerr << "  -V      :verbose output" << std::endl;
				std::cerr << "  -T <i>  :execute only test with index <i>" << std::endl;
				std::cerr << "  -G <s>  :enable debug trace of <s>" << std::endl;
				std::cerr << "  -K      :keep data, do not cleanup" << std::endl;
			}
			else if (std::strcmp( argv[ii], "-K") == 0)
			{
				do_cleanup = false;
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
		g_errorhnd = strus::createErrorBuffer_standard( stderr, 2/*maxNofThreads*/, g_dbgtrace);
		if (!g_errorhnd) {std::cerr << "FAILED " << "strus::createErrorBuffer_standard" << std::endl; return -1;}
		g_fileLocator = strus::createFileLocator_std( g_errorhnd);
		if (!g_fileLocator) {std::cerr << "FAILED " << "strus::createFileLocator_std" << std::endl; return -1;}

		strus::Reference<strus::QueryProcessorInterface> qpi = strus::createQueryProcessor( g_fileLocator, g_errorhnd);
		if (!qpi.get())
		{
			throw strus::runtime_error("failed to create query processor instance");
		}
		if (g_errorhnd->hasError())
		{
			throw strus::runtime_error("error initializing strus objects");
		}
		unsigned int ti=test_index?test_index:1;
		for (;!rt;++ti)
		{
			switch (ti)
			{
				case 1: RUN_TEST( ti, PlainSingleTermQuery, qpi.get(), rt ) break;
				case 2: RUN_TEST( ti, SingleTermQueryWithExclusion, qpi.get(), rt ) break;
				case 3: RUN_TEST( ti, SingleTermQueryWithRestriction, qpi.get(), rt ) break;
				case 4: RUN_TEST( ti, SingleTermQueryWithRestrictionInclMetadata, qpi.get(), rt ) break;
				case 5: RUN_TEST( ti, SingleTermQueryWithSelectionAndRestriction, qpi.get(), rt ) break;
				default: goto TESTS_DONE;
			}
			if (test_index) break;
		}
		if (g_errorhnd->hasError())
		{
			const char* errmsg = g_errorhnd->fetchError();
			std::cerr << "Error: " << (errmsg?errmsg:"") << std::endl;
			rt = -1;
		}
	}
	catch (const std::runtime_error& err)
	{
		const char* errmsg = g_errorhnd->fetchError();
		std::cerr << "Error: " << err.what() << (errmsg?": ":0) << (errmsg?errmsg:"") << std::endl;
		rt = -1;
	}
	catch (const std::bad_alloc& err)
	{
		std::cerr << "Out of memory" << std::endl;
		return -1;
	}
TESTS_DONE:
	std::vector<strus::DebugTraceMessage> msglist = g_dbgtrace->fetchMessages();
	if (!msglist.empty())
	{
		std::cerr << "Debug trace:" << std::endl;
		std::vector<strus::DebugTraceMessage>::const_iterator
			mi = msglist.begin(), me = msglist.end();
		for (; mi != me; ++mi)
		{
			std::cerr << strus::string_format( "%s %s %s '%s'",
					mi->typeName(), mi->id(), mi->component(), mi->content().c_str())
				<< std::endl;
		}
	}
	if (do_cleanup)
	{
		destroyStorage( "path=storage");
	}
	delete g_fileLocator;
	delete g_errorhnd;
	return rt;
}


