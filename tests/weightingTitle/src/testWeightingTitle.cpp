/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "storageDef.hpp"
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
#include "strus/storageDocumentInterface.hpp"
#include "strus/storageDocumentUpdateInterface.hpp"
#include "strus/storageDumpInterface.hpp"
#include "strus/structIteratorInterface.hpp"
#include "strus/valueIteratorInterface.hpp"
#include "strus/constants.hpp"
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
#include <utility>
#include <limits>
#include <list>
#include <algorithm>
#include <cmath>

static strus::ErrorBufferInterface* g_errorhnd = 0;
static strus::FileLocatorInterface* g_fileLocator = 0;
static int g_verbosity = 0;
static bool g_dumpCollection = false;
static strus::PseudoRandom g_random;

typedef strus::test::Storage Storage;

struct TitleTreeNode
{
	std::vector<int> featar;
	std::list<TitleTreeNode> chld;
	strus::IndexRange field;

	TitleTreeNode()
		:featar(),chld(),field(){}
	TitleTreeNode( const std::vector<int>& featar_, strus::Index startpos_)
		:featar(featar_),chld(),field( startpos_, startpos_ + featar_.size()){}
	TitleTreeNode( const TitleTreeNode& o)
		:featar(o.featar),chld(o.chld),field(o.field){}

	void add( const TitleTreeNode& nd)
		{chld.push_back(nd); field.setEnd( chld.back().field.end());}
};

static TitleTreeNode createTitleTree( int nofChilds, int nofTerms, int nofFeatures, int depth, strus::Index startpos)
{
	std::vector<int> featar;
	int fi = 0, fe = g_random.get( 1, nofFeatures);
	for (; fi != fe; ++fi)
	{
		featar.push_back( g_random.get( 0, nofTerms));
	}
	if (startpos <= 0) startpos = 1;
	TitleTreeNode rt( featar, startpos);
	startpos += rt.featar.size();
	if (startpos <= rt.field.start() || startpos >= strus::Constants::storage_max_position_info()) throw std::runtime_error(_TXT("test tree too complex"));

	if (depth > 0)
	{
		int mi = 0, me = g_random.get( 0, nofChilds);
		for (; mi != me; ++mi)
		{
			rt.add( createTitleTree( nofChilds, nofTerms, nofFeatures, depth-1, startpos));
			startpos = rt.field.end();
		}
	}
	return rt;
}

struct Query
{
	double hierarchyWeightFactor;
	std::vector<int> features;

	Query( double hierarchyWeightFactor_, const std::vector<int>& features_)
		:hierarchyWeightFactor(hierarchyWeightFactor_),features(features_){}
	Query( const Query& o)
		:hierarchyWeightFactor(o.hierarchyWeightFactor),features(o.features){}

	static Query createRandom( const TitleTreeNode& tree)
	{
		double hierarchyWeightFactor_ = ((double)g_random.get( 0, 100000) / 100000.0) * 0.3 + 0.7;
		std::vector<int> features_;

		std::vector<const TitleTreeNode*> completeNodes;
		completeNodes.push_back( &tree);
		while (g_random.get( 0, 1+completeNodes.size()) == 0)
		{
			int cidx = g_random.get( 0, completeNodes.back()->chld.size());
			std::list<TitleTreeNode>::const_iterator
				ci = completeNodes.back()->chld.begin(),
				ce = completeNodes.back()->chld.end();
			for (; cidx > 0; --cidx,++ci){}
			completeNodes.push_back( &*ci);
		}
		const TitleTreeNode* finalNode = completeNodes.back();
		completeNodes.pop_back();

		std::vector<const TitleTreeNode*>::const_iterator
			si = completeNodes.begin(), se = completeNodes.end();
		for (; si != se; ++si)
		{
			std::vector<int> fi = (*si)->featar.begin(), fe = (*si)->featar.end();
			for (; fi != fe; ++fi)
			{
				features_.push_back( *fi);
			}
		}
		int nofFinal = g_random.get( 1, finalNode->featar.size());
		std::vector<int> fi = finalNode->featar.begin(), fe = finalNode->featar.end();
		for (; fi != fe && nofFinal > 0; ++fi,--nofFinal)
		{
			features_.push_back( *fi);
		}
		return Query( hierarchyWeightFactor_, features_);
	}

	std::vector<strus::WeightedField> answer( const TitleTreeNode& tree)
	{
		std::vector<const TitleTreeNode*> stk;
		stk.push_back( &tree);
		while (!stk.empty())
		{
			const TitleTreeNode* nd = stk.back();
			stk.pop_back();
			std::vector<int> features
		}
	}
};

static void fillStructureList( std::vector<strus::test::StructureDef>& structurelist, const TitleTreeNode& node)
{
	strus::IndexRange header( node.field.start(), node.field.start() + node.featar.size());
	std::list<TitleTreeNode>::const_iterator ci = node.chld.begin(), ce = node.chld.end();
	if (ci == ce)
	{
		structurelist.push_back( strus::test::StructureDef( "title", header, header));
	}
	else
	{
		for (; ci != ce; ++ci)
		{
			structurelist.push_back( strus::test::StructureDef( "title", header, ci->field));
			fillStructureList( structurelist, *ci);
		}
	}
}

static void fillFeatureList( std::vector<strus::test::Feature>& featurelist, const TitleTreeNode& node)
{
	std::vector<int>::const_iterator fi = node.featar.begin(), fe = node.featar.end();
	for (int fidx=0; fi != fe; ++fi,++fidx)
	{
		strus::Index pos = node.field.start() + fidx;
		std::string word = strus::string_format( "f%d", *fi);
		featurelist.push_back( strus::test::Feature( strus::test::Feature::SearchIndex, "word", word, pos));
		std::string orig = strus::string_format( "F%d", *fi);
		featurelist.push_back( strus::test::Feature( strus::test::Feature::ForwardIndex, "orig", orig, pos));
	}
	std::list<TitleTreeNode>::const_iterator ci = node.chld.begin(), ce = node.chld.end();
	for (; ci != ce; ++ci)
	{
		fillFeatureList( featurelist, *ci);
	}
}

struct Document
{
	std::string docid;
	TitleTreeNode titletree;
	std::vector<strus::test::StructureDef> structurelist;
	std::vector<strus::test::Feature> featurelist;

	Document( int didx, const TitleTreeNode& titletree_)
		:docid(strus::string_format("D%d", didx)),titletree(titletree_),structurelist()
	{
		fillStructureList( structurelist, titletree);
		fillFeatureList( featurelist, titletree);
	}

	Document( const Document& o)
		:docid(o.docid),titletree(o.titletree)
		,structurelist(o.structurelist),featurelist(o.featurelist){}

	static Document createRandom( int didx, int nofTerms, int nofNodes)
	{
		enum {DepthDistributionSize=30};
		static const int depthDistribution[ DepthDistributionSize] = {0,1,1,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,5,5,5,6,6,7};
		enum {FeatsDistributionSize=30};
		static const int featsDistribution[ FeatsDistributionSize] = {1,1,1,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,5,5,5,6,6,7};

		enum {NofDist=4};
		int depth = depthDistribution[ g_random.get( 0, DepthDistributionSize)];
		int nofFeatures = featsDistribution[ g_random.get( 0, FeatsDistributionSize)];
		float nofChilds = (depth == 0) ? nofNodes : std::pow( (double)nofNodes, 1.0/(double)depth);
		
		TitleTreeNode titletree_ = createTitleTree( nofChilds, nofTerms, nofFeatures, depth, 0/*startpos*/);
		return Document( didx+1, titletree_);
	}

	void build( strus::StorageDocumentInterface* doc) const
	{
		std::vector<strus::test::Feature>::const_iterator
			fi = featurelist.begin(), fe = featurelist.end();
		for (; fi != fe; ++fi)
		{
			switch (fi->kind)
			{
				case strus::test::Feature::SearchIndex:
					doc->addSearchIndexTerm( fi->type, fi->value, fi->pos);
					break;
				case strus::test::Feature::ForwardIndex:
					doc->addForwardIndexTerm( fi->type, fi->value, fi->pos);
					break;
				case strus::test::Feature::Attribute:
					throw std::runtime_error("no attributes defined here");
				case strus::test::Feature::MetaData:
					throw std::runtime_error("no metadata defined here");
			}
		}
		std::vector<strus::test::StructureDef>::const_iterator
			xi = structurelist.begin(), xe = structurelist.end();
		for (; xi != xe; ++xi)
		{
			doc->addSearchIndexStructure( xi->name(), xi->header(), xi->content());
		}
		doc->done();
	}
};

struct Collection
{
	std::vector<Document> doclist;

	Collection( int nofDocuments, int nofTerms, int nofNodes)
		:doclist()
	{
		int di = 0, de = nofDocuments;
		for (; di != de; ++di)
		{
			doclist.push_back( Document::createRandom( di, nofTerms, nofNodes));
		}
	}
	Collection( const Collection& o)
		:doclist(o.doclist){}

	void insert( strus::StorageClientInterface* storage, const std::string& selectDocid)
	{
		strus::local_ptr<strus::StorageTransactionInterface> transaction( storage->createTransaction());
		try
		{
			std::vector<Document>::const_iterator di = doclist.begin(), de = doclist.end();
			for (; di != de; ++di)
			{
				if (di->docid == selectDocid)
				{
					strus::local_ptr<strus::StorageDocumentInterface>
						doc( transaction->createDocument( di->docid));
					if (!doc.get()) throw strus::runtime_error("error creating document to insert");

					di->build( doc.get());
				}
			}
		}
		catch (const std::runtime_error& err)
		{
			throw std::runtime_error( err.what());
		}
		if (g_errorhnd->hasError())
		{
			throw strus::runtime_error( "insert failed: %s", g_errorhnd->fetchError());
		}
		if (!transaction->commit())
		{
			throw strus::runtime_error( "transaction failed: %s", g_errorhnd->fetchError());
		}
	}

	void insert( strus::StorageClientInterface* storage, int commitSize)
	{
		strus::local_ptr<strus::StorageTransactionInterface> transaction( storage->createTransaction());
		if (g_verbosity >= 1) {fprintf( stderr, "\n"); fflush( stderr);}
		int didx = 0;

		try
		{
			std::vector<Document>::const_iterator di = doclist.begin(), de = doclist.end();
			for (; di != de; ++di,++didx)
			{
				if (commitSize > 0 && didx > 0 && didx % commitSize == 0)
				{
					if (g_errorhnd->hasError())
					{
						throw strus::runtime_error( "insert failed: %s", g_errorhnd->fetchError());
					}
					if (!transaction->commit())
					{
						throw strus::runtime_error( "transaction failed: %s", g_errorhnd->fetchError());
					}
					if (g_verbosity >= 1) {std::fprintf( stderr, "\rinserted %d documents", didx); fflush( stderr);}
				}
				strus::local_ptr<strus::StorageDocumentInterface>
					doc( transaction->createDocument( di->docid));
				if (!doc.get()) throw strus::runtime_error("error creating document to insert");

				di->build( doc.get());
			}
		}
		catch (const std::runtime_error& err)
		{
			if (g_verbosity >= 1) std::fprintf( stderr, "\rinserted %d documents\n", didx);
			throw std::runtime_error( err.what());
		}
		if (g_errorhnd->hasError())
		{
			throw strus::runtime_error( "insert failed: %s", g_errorhnd->fetchError());
		}
		if (!transaction->commit())
		{
			throw strus::runtime_error( "transaction failed: %s", g_errorhnd->fetchError());
		}
		if (g_verbosity >= 1) std::fprintf( stderr, "\rinserted %d documents\n", didx);
	}
};



static std::vector<std::string> randomQuery()
{
	
}

static void testWeightingTitle( int nofDocuments, int nofTerms, int nofNodes, int commitSize, int nofQueryies, const std::string& selectDocid)
{
	Storage storage( g_fileLocator, g_errorhnd);
	storage.open( "path=storage", true);
	Collection collection( nofDocuments, nofTerms, nofNodes);

	if (selectDocid.empty())
	{
		collection.insert( storage.sci.get(), commitSize);
	}
	else
	{
		collection.insert( storage.sci.get(), selectDocid);
	}
}


static void printUsage()
{
	std::cerr << "usage: testWeightingTitle [options] <nofdocs> <nofterms> <nofnodes> <commitsize> <nofqry>" << std::endl;
	std::cerr << "description: Inserts a collection of documents with hierachical title" << std::endl;
	std::cerr << "             structures and search for example title hierarchy references." << std::endl;
	std::cerr << "options:" << std::endl;
	std::cerr << "  -h             :print usage" << std::endl;
	std::cerr << "  -V             :increment verbosity (repeating)" << std::endl;
	std::cerr << "  -D             :dump collection created" << std::endl;
	std::cerr << "  -K             :keep artefacts, do not clean up" << std::endl;
	std::cerr << "  -W <seed>      :sepecify pseudo random number generator seed (int)" << std::endl;
	std::cerr << "  -F <docid>     :only process insert of document with id <docid>" << std::endl;
	std::cerr << "<nofdocs>        :number of documents inserted" << std::endl;
	std::cerr << "<nofterms>       :number of variants of document terms" << std::endl;
	std::cerr << "<nofnodes>       :average number of title nodes per document" << std::endl;
	std::cerr << "<commitsize>     :number of documents inserted per transaction" << std::endl;
	std::cerr << "<nofqry>         :number of random queries to verify" << std::endl;
}

int main( int argc, const char* argv[])
{
	int rt = 0;
	bool do_cleanup = true;
	int argi = 1;
	std::string selectDocid;
	for (; argc > argi; ++argi)
	{
		if (std::strcmp( argv[argi], "-K") == 0)
		{
			do_cleanup = false;
		}
		else if (std::strcmp( argv[argi], "-W") == 0)
		{
			if (++argi == argc) throw std::runtime_error("argument expected for option -W (random seed)");
			if (argv[argi][0] < '0' || argv[argi][0] > '9') throw std::runtime_error("non negative integer argument expected for option -W (random seed)");
			g_random.init( atoi( argv[argi]));
		}
		else if (std::strcmp( argv[argi], "-F") == 0)
		{
			if (++argi == argc) throw std::runtime_error("argument expected for option -F (docid)");
			selectDocid = argv[argi];
		}
		else if (std::strcmp( argv[argi], "-V") == 0)
		{
			g_verbosity += 1;
		}
		else if (std::strcmp( argv[argi], "-VV") == 0)
		{
			g_verbosity += 2;
		}
		else if (std::strcmp( argv[argi], "-D") == 0)
		{
			g_dumpCollection = true;
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
		if (argi + 5 > argc)
		{
			printUsage();
			throw std::runtime_error( "too few arguments");
		}
		if (argi + 5 < argc)
		{
			printUsage();
			throw std::runtime_error( "too many arguments");
		}
		int ai = 0;
		int nofDocuments = strus::numstring_conv::toint( argv[ argi+ai], std::strlen(argv[ argi+ai]), std::numeric_limits<int>::max());
		++ai;
		int nofTerms = strus::numstring_conv::toint( argv[ argi+ai], std::strlen(argv[ argi+ai]), std::numeric_limits<int>::max());
		++ai;
		int nofNodes = strus::numstring_conv::toint( argv[ argi+ai], std::strlen(argv[ argi+ai]), std::numeric_limits<int>::max());
		++ai;
		int commitSize = strus::numstring_conv::toint( argv[ argi+ai], std::strlen(argv[ argi+ai]), std::numeric_limits<int>::max());
		++ai;
		int nofQueryies = strus::numstring_conv::toint( argv[ argi+ai], std::strlen(argv[ argi+ai]), std::numeric_limits<int>::max());
		++ai;

		testWeightingTitle( nofDocuments, nofTerms, nofNodes, commitSize, nofQueryies, selectDocid);
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


