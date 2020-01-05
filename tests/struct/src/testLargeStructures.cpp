/*
 * Copyright (c) 2019 Patrick P. Frey
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
#include <algorithm>

static strus::ErrorBufferInterface* g_errorhnd = 0;
static strus::FileLocatorInterface* g_fileLocator = 0;
static bool g_verbose = false;
static bool g_dumpCollection = false;
static const char* g_structureName = "STU";
static strus::PseudoRandom g_random;

typedef strus::test::Storage Storage;

static std::vector<strus::IndexRange> createRandomVariationMembers( 
		int nofMembers, int start, int startSize, int startOfs, int variations)
{
	if (startOfs < startSize)
	{
		throw std::logic_error("bad parameter for creating member (variation)");
	}
	std::vector<strus::IndexRange> rt;
	int mi = 0, me = nofMembers;
	for (; mi != me; ++mi)
	{
		if (start + startSize >= strus::Constants::storage_max_position_info()-1) break;
		rt.push_back( strus::IndexRange( start, start+startSize));
		start += startOfs;

		if (rt.back().end() < rt.back().start())
		{
			throw std::logic_error("creating bad member (variation)");
		}
		if (0==variations || 0==g_random.get( 0, variations))
		{
			startSize += (int)g_random.get( 0, 3)-1;
			if (startSize <= 0) startSize = g_random.get( 1, 3);
			startOfs += (int)g_random.get( 0, 3)-1;
			if (startOfs < startSize) startOfs = startSize + g_random.get( 0, 2);
		}
	}
	return rt;
}

static std::vector<strus::IndexRange> createRandomDistributedIndexRanges( int nn, int start)
{
	std::vector<strus::IndexRange> rt;
	std::set<int> startSet;
	while ((int)startSet.size() <= nn)
	{
		startSet.insert( g_random.get( 0, strus::Constants::storage_max_position_info()-start-1) + start);
	}
	std::set<int>::const_iterator si = startSet.begin(), se = startSet.end(), sn = startSet.begin();
	for (++sn; sn != se; ++si,++sn)
	{
		rt.push_back( strus::IndexRange( *si, g_random.get( *si, *sn)+1));
		if (rt.back().end() < rt.back().start())
		{
			throw std::logic_error("creating bad member (random)");
		}
	}
	return rt;
}

static std::vector<strus::test::StructureDef> createRandomStructure(
		const std::string& name,
		int headerStart, int headerEnd,
		int nofMembers, int memberStart, int memberStartSize, int memberStartOfs,
		int variations)
{
	std::vector<strus::test::StructureDef> rt;
	if (headerEnd < headerStart)
	{
		throw std::logic_error("creating bad header");
	}
	strus::IndexRange header( headerStart, headerEnd);

	std::vector<strus::IndexRange> members
		= variations > 0
		? createRandomVariationMembers( nofMembers, memberStart, memberStartSize, memberStartOfs, variations)
		: createRandomDistributedIndexRanges( nofMembers, memberStart);
	std::vector<strus::IndexRange>::const_iterator mi = members.begin(), me = members.end();
	for (; mi != me; ++mi)
	{
		rt.push_back( strus::test::StructureDef( name, header, *mi));
	}
	return rt;
}

static std::vector<strus::test::StructureDef> createRandomStructures( int nofStructures, int maxNofMembers)
{
	enum {NofDim=5};
	static const int dim[] = {1,10,100,1000,10000};

	std::vector<strus::test::StructureDef> rt;
	std::vector<strus::IndexRange> headerList = createRandomDistributedIndexRanges( nofStructures, 1);
	std::vector<strus::IndexRange>::const_iterator hi = headerList.begin(), he = headerList.end();
	for (int hidx=0; hi != he; ++hi,++hidx)
	{
		int nofMembers = g_random.get( 1, maxNofMembers);
		int memberStart = g_random.get( 1, dim[ g_random.get( 0, NofDim)]+1);
		int memberStartSize = g_random.get( 1, dim[ g_random.get( 0, NofDim-1)]+1);
		int memberStartOfs = memberStartSize + dim[ g_random.get( 0, NofDim-2)];
		int variations = g_random.get( 0, hidx+1);

		std::vector<strus::test::StructureDef> stlist
			= createRandomStructure( 
				g_structureName, hi->start(), hi->end(),
				nofMembers, memberStart, memberStartSize, memberStartOfs,
				variations);
		rt.insert( rt.end(), stlist.begin(), stlist.end());
	}
	return rt;
}

static void buildDocument( strus::StorageDocumentInterface* doc, const std::vector<strus::test::StructureDef>& structurelist)
{
	std::vector<strus::test::StructureDef>::const_iterator
		xi = structurelist.begin(), xe = structurelist.end();
	for (; xi != xe; ++xi)
	{
		doc->addSearchIndexStructure( xi->name(), xi->header(), xi->content());
	}
	doc->done();
}

struct Document
{
	std::string docid;
	std::vector<strus::test::StructureDef> structurelist;

	Document( int didx, const std::vector<strus::test::StructureDef>& structurelist_)
		:docid(strus::string_format("D%d", didx)),structurelist(structurelist_){}
	Document( const Document& o)
		:docid(o.docid),structurelist(o.structurelist){}

	static Document createRandom( int didx, int nofStructures, int maxNofMembers)
	{
		return Document( didx+1, createRandomStructures( nofStructures, maxNofMembers));
	}

	void eliminateRandom( float fraction)
	{
		if (fraction >= 1.0)
		{
			structurelist.clear();
			return;
		}
		if (fraction <= 0.0)
		{
			return;
		}
		std::vector<strus::test::StructureDef> structurelist_new;
		std::vector<strus::test::StructureDef>::const_iterator
			si = structurelist.begin(), se = structurelist.end();
		for (; si != se; ++si)
		{
			if (g_random.get( 1, std::numeric_limits<int>::max()) <= (int)(fraction * std::numeric_limits<int>::max()))
			{
				continue;
			}
			structurelist_new.push_back( *si);
		}
		structurelist_new.swap( structurelist);
	}
};

struct Collection
{
	std::vector<Document> doclist;

	Collection( int nofDocuments, int nofStructures, int maxNofMembers)
		:doclist()
	{
		int di = 0, de = nofDocuments;
		for (; di != de; ++di)
		{
			doclist.push_back( Document::createRandom( di, nofStructures, maxNofMembers));
		}
	}
	Collection( const Collection& o)
		:doclist(o.doclist){}

	void insert( strus::StorageClientInterface* storage, int commitSize)
	{
		strus::local_ptr<strus::StorageTransactionInterface> transaction( storage->createTransaction());
		if (g_verbose) {fprintf( stderr, "\n"); fflush( stderr);}
		int didx = 0;

		try
		{
			std::vector<Document>::const_iterator di = doclist.begin(), de = doclist.end();
			for (; di != de; ++di,++didx)
			{
				if (g_verbose) {std::fprintf( stderr, "\rinserted %d documents", didx); fflush( stderr);}
	
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
				}
				strus::local_ptr<strus::StorageDocumentInterface>
					doc( transaction->createDocument( di->docid));
				if (!doc.get()) throw strus::runtime_error("error creating document to insert");

				buildDocument( doc.get(), di->structurelist);
			}
		}
		catch (const std::runtime_error& err)
		{
			if (g_verbose) std::fprintf( stderr, "\rinserted %d documents\n", didx);
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
		if (g_verbose) std::fprintf( stderr, "\rinserted %d documents\n", didx);
	}

	void eliminateRandom( float fraction)
	{
		if (fraction >= 1.0)
		{
			doclist.clear();
			return;
		}
		if (fraction <= 0.0)
		{
			return;
		}
		std::vector<Document> doclist_new;
		std::vector<Document>::const_iterator
			di = doclist.begin(), de = doclist.end();
		for (; di != de; ++di)
		{
			if (g_random.get( 1, std::numeric_limits<int>::max()) <= (int)(fraction * std::numeric_limits<int>::max()))
			{
				continue;
			}
			doclist_new.push_back( *di);
			doclist_new.back().eliminateRandom( fraction);
		}
		doclist_new.swap( doclist);
	}

	void dump( std::ostream& out)
	{
		std::vector<Document>::const_iterator di = doclist.begin(), de = doclist.end();
		for (; di != de; ++di)
		{
			out << "DOC " << di->docid << ":" << std::endl;
			std::vector<strus::test::StructureDef>::const_iterator
				si = di->structurelist.begin(),
				se = di->structurelist.end();
			while (si != se)
			{
				strus::IndexRange hdr = si->header();
				out << strus::string_format( "\t[%d,%d] ->", (int)hdr.start(), (int)hdr.end());
				for (; si != se && si->header() == hdr; ++si)
				{
					out << strus::string_format( " [%d,%d]", (int)si->content().start(), (int)si->content().end());
				}
				out << std::endl;
			}
		}
	}

	struct Answer
	{
		strus::IndexRange header;
		strus::IndexRange content;

		Answer( const strus::IndexRange& header_, const strus::IndexRange& content_)
			:header(header_),content(content_){}
		Answer( const Answer& o)
			:header(o.header),content(o.content){}
	};

	struct Query
	{
		std::string docid;
		strus::Index structno;
		strus::Index headerpos;
		strus::Index contentpos;

		Query( const std::string& docid_, strus::Index headerpos_, strus::Index contentpos_)
			:docid(docid_),structno(1),headerpos(headerpos_),contentpos(contentpos_){}
		Query( const Query& o)
			:docid(o.docid),structno(o.structno),headerpos(o.headerpos),contentpos(o.contentpos){}
	};

	struct QueryAnswerPair
	{
		Query query;
		Answer answer;

		QueryAnswerPair( const Query& query_, const Answer& answer_)
			:query(query_),answer(answer_){}
		QueryAnswerPair( const QueryAnswerPair& o)
			:query(o.query),answer(o.answer){}
	};

	strus::Index randomIndex( const strus::IndexRange& base) const
	{
		enum {NofDim=5};
		static const strus::Index dim[ NofDim] = {10,100,1000,10000,strus::Constants::storage_max_position_info()};

		enum QueryCase {CaseStart,CaseBeforeStart,CaseAfterStart,CaseEnd,CaseBeforeEnd,CaseAfterEnd,CaseRandom};
		enum {NofQueryCases=CaseRandom+1};
		QueryCase queryCase = (QueryCase)g_random.get( CaseStart, NofQueryCases);
		switch (queryCase)
		{
			case CaseStart:
				return base.start();
			case CaseBeforeStart:
				return base.start()-1;
			case CaseAfterStart:
				return base.start()+1;
			case CaseEnd:
				return base.end();
			case CaseBeforeEnd:
				return base.end()-1;
			case CaseAfterEnd:
				return base.end()+1;
			case CaseRandom:
				return g_random.get( 0, dim[ g_random.get( 0, NofDim)]);
		}
		throw std::runtime_error("missing query case");
	}

	QueryAnswerPair randomQueryAnswer() const
	{
		strus::Index docidx = g_random.get( 0, doclist.size());
		const Document& doc = doclist[ docidx];
		const std::string& docid = doc.docid;
		std::size_t stuidx = g_random.get( 0, doc.structurelist.size());
		const strus::test::StructureDef& studef = doc.structurelist[ stuidx];
		strus::IndexRange hdr = studef.header();
		strus::IndexRange mbr = studef.content();

		strus::Index headerpos = randomIndex( hdr);
		strus::Index contentpos = randomIndex( mbr);
		strus::IndexRange answer_hdr;
		strus::IndexRange answer_mbr;

		if (headerpos >= hdr.start() && headerpos < hdr.end())
		{
			answer_hdr = hdr;
		}
		else
		{
			std::size_t hi = 0, he = doc.structurelist.size();
			for (; hi != he && doc.structurelist[hi].header().end() <= headerpos; ++hi){}
			if (hi != he)
			{
				answer_hdr = doc.structurelist[hi].header();
			}
		}
		int mi = 0, me = doc.structurelist.size();
		if (answer_hdr == hdr && contentpos >= mbr.start() && contentpos < mbr.end())
		{
			answer_mbr = mbr;
			mi = stuidx;
		}
		else
		{
			for (; mi != me && doc.structurelist[mi].header() != answer_hdr; ++mi){}
			for (; mi != me && doc.structurelist[mi].header() == answer_hdr && doc.structurelist[mi].content().end() <= contentpos; ++mi){}
			if (mi != me && doc.structurelist[mi].header() != answer_hdr) mi = me;
		}
		if (mi != me)
		{
			answer_mbr = doc.structurelist[mi].content();
			int start = mi;
			for (mi = start+1; mi != me
				&& doc.structurelist[mi].header() == answer_hdr
				&& doc.structurelist[mi].content().start() == answer_mbr.end(); ++mi)
			{
				answer_mbr = strus::IndexRange( answer_mbr.start(), doc.structurelist[mi].content().end());
			}
			for (mi = start-1; mi >= 0
				&& doc.structurelist[mi].header() == answer_hdr
				&& doc.structurelist[mi].content().end() == answer_mbr.start(); --mi)
			{
				answer_mbr = strus::IndexRange( doc.structurelist[mi].content().start(), answer_mbr.end());
			}
		}
		return QueryAnswerPair( Query( docid, headerpos, contentpos), Answer( answer_hdr, answer_mbr));
	}
};

static std::pair<strus::IndexRange,int> findQuerySource( strus::StructIteratorInterface* sitr, strus::Index structno, strus::Index pos)
{
	std::pair<strus::IndexRange,int> rt;
	int li = 0, le = sitr->levels();
	for (; li != le; ++li)
	{
		strus::IndexRange source = sitr->skipPos( li, pos);
		while (source.defined())
		{
			strus::StructIteratorInterface::StructureLinkArray lnkar = sitr->links( li);
			int si = 0, se = lnkar.nofLinks();
			for (; si != se; ++si)
			{
				strus::StructIteratorInterface::StructureLink lnk = lnkar.link( si);
				if (lnk.header() == true && lnk.structno() == structno)
				{
					if (!rt.first.defined() || (source.defined() && rt.first.end() < source.end()))
					{
						rt.first = source;
						rt.second = lnk.index();
						break;
					}
				}
			}
			source = sitr->skipPos( li, pos = source.end());
		}
	}
	return rt;
}

static strus::IndexRange findQuerySink( strus::StructIteratorInterface* sitr, strus::Index structno, int structidx, strus::Index pos)
{
	int li = 0, le = sitr->levels();
	for (; li != le; ++li)
	{
		strus::IndexRange sink = sitr->skipPos( li, pos);
		while (sink.defined())
		{
			strus::StructIteratorInterface::StructureLinkArray lnkar = sitr->links( li);
			int si = 0, se = lnkar.nofLinks();
			for (; si != se; ++si)
			{
				strus::StructIteratorInterface::StructureLink lnk = lnkar.link( si);
				if (lnk.header() == false && lnk.structno() == structno && lnk.index() == structidx)
				{
					return sink;
				}
			}
			sink = sitr->skipPos( li, pos = sink.end());
		}
	}
	return strus::IndexRange();
}

static void verifyQueryAnswer( Storage& storage, strus::StructIteratorInterface* sitr, const Collection::QueryAnswerPair& qa)
{
	strus::Index docno = storage.sci->documentNumber( qa.query.docid);
	if (!docno) throw strus::runtime_error("document id unknown: %s", qa.query.docid.c_str());
	sitr->skipDoc( docno);

	std::pair<strus::IndexRange,int> stu = findQuerySource( sitr, qa.query.structno, qa.query.headerpos);

	strus::Index structno = qa.query.structno;
	int structidx = stu.second;
	strus::IndexRange source = stu.first;
	strus::IndexRange sink;

	if (source.defined())
	{
		sink = findQuerySink( sitr, structno, structidx, qa.query.contentpos);
	}
	if (source != qa.answer.header)
	{
		throw strus::runtime_error(
			"answer header [%d,%d]"
			" searching %d in document %d (%s)"
			" does not match expected [%d,%d]",
			(int)source.start(), (int)source.end(),
			(int)qa.query.headerpos, docno, qa.query.docid.c_str(),
			(int)qa.answer.header.start(), (int)qa.answer.header.end());
	}
	if (sink != qa.answer.content)
	{
		throw strus::runtime_error(
			"answer content [%d,%d]"
			" searching %d in structure with header [%d,%d] in document %d (%s)"
			" does not match expected [%d,%d]",
			(int)sink.start(), (int)sink.end(),
			(int)qa.query.contentpos, (int)source.start(), (int)source.end(), docno, qa.query.docid.c_str(),
			(int)qa.answer.content.start(), (int)qa.answer.content.end());
	}
}

static void testLargeStructures( int nofCycles, int nofDocuments, int nofStructures, int maxNofMembers, int commitSize, int nofQueryies)
{
	Storage storage( g_fileLocator, g_errorhnd);
	storage.open( "path=storage", true);
	Collection collection( nofDocuments, nofStructures, maxNofMembers);

	if (nofCycles > 2)
	{
		int ci = 0, ce = nofCycles-1;
		for (; ci != ce; ++ci)
		{
			Collection subcollection( collection);
			subcollection.eliminateRandom( (float)(ce-(ci+1)) / nofCycles);
			subcollection.insert( storage.sci.get(), commitSize);
		}
	}
	collection.insert( storage.sci.get(), commitSize);
	if (g_dumpCollection) collection.dump( std::cout);

	int qi = 0, qe = nofQueryies;
	strus::local_ptr<strus::StructIteratorInterface> sitr( storage.sci->createStructIterator());
	if (!sitr.get()) throw std::runtime_error("failed to create structure iterator");

	if (g_verbose) {fprintf( stderr, "\n"); fflush( stderr);}
	try
	{
		int qmodta = std::sqrt( nofQueryies / 10);
		int qmod = 1;
		while (qmod < qmodta) qmod *= 10;

		for (; qi != qe; ++qi)
		{
			if (qi && qi % qmod == 0)
			{
				if (g_verbose) fprintf( stderr, "\rissued %d queries", qi);
			}
			verifyQueryAnswer( storage, sitr.get(), collection.randomQueryAnswer());
		}
	}
	catch (const std::runtime_error& err)
	{
		if (g_verbose) {fprintf( stderr, "\rissued %d queries\n", qi); fflush( stderr);}
		throw std::runtime_error( err.what());
	}
	if (g_verbose) {fprintf( stderr, "\rissued %d queries\n", qi); fflush( stderr);}
}

static void printUsage()
{
	std::cerr << "usage: testLargeStructures [options] <cycles> <nofdocs> <nofst> <nofmb> <commitsize> <nofqry>" << std::endl;
	std::cerr << "description: Inserts a collection of documents with large structures" << std::endl;
	std::cerr << "             and verify the stored data." << std::endl;
	std::cerr << "options:" << std::endl;
	std::cerr << "  -h             :print usage" << std::endl;
	std::cerr << "  -V             :verbose output" << std::endl;
	std::cerr << "  -D             :dump collection created" << std::endl;
	std::cerr << "  -K             :keep artefacts, do not clean up" << std::endl;
	std::cerr << "<cycles>      :number of (re-)insert cycles" << std::endl;
	std::cerr << "<nofdocs>     :number of documents inserted in each (re-)insert cycle" << std::endl;
	std::cerr << "<nofst>       :maximum number of structures per document" << std::endl;
	std::cerr << "<nofmb>       :maximum number of members per structure" << std::endl;
	std::cerr << "<commitsize>  :number of documents inserted per transaction" << std::endl;
	std::cerr << "<nofqry>      :number of random queries to verify stored structures" << std::endl;
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
		if (argi + 6 > argc)
		{
			printUsage();
			throw std::runtime_error( "too few arguments");
		}
		if (argi + 6 < argc)
		{
			printUsage();
			throw std::runtime_error( "too many arguments");
		}
		int nofCycles = strus::numstring_conv::toint( argv[ argi+0], std::strlen(argv[ argi+0]), std::numeric_limits<int>::max());
		int nofDocuments = strus::numstring_conv::toint( argv[ argi+1], std::strlen(argv[ argi+1]), std::numeric_limits<int>::max());
		int nofStructures = strus::numstring_conv::toint( argv[ argi+2], std::strlen(argv[ argi+2]), std::numeric_limits<int>::max());
		int maxNofMembers = strus::numstring_conv::toint( argv[ argi+3], std::strlen(argv[ argi+3]), std::numeric_limits<int>::max());
		int commitSize = strus::numstring_conv::toint( argv[ argi+4], std::strlen(argv[ argi+4]), std::numeric_limits<int>::max());
		int nofQueryies = strus::numstring_conv::toint( argv[ argi+5], std::strlen(argv[ argi+5]), std::numeric_limits<int>::max());

		testLargeStructures( nofCycles, nofDocuments, nofStructures, maxNofMembers, commitSize, nofQueryies);
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


