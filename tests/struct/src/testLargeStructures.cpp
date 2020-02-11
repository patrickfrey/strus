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
#include "strus/structureIteratorInterface.hpp"
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
static const char* g_structureName = "STU";
static strus::PseudoRandom g_random;

typedef strus::test::Storage Storage;

struct RangeTreeNode
{
	strus::IndexRange range;
	std::list<RangeTreeNode> chld;

	RangeTreeNode()
		:range(),chld(){}
	RangeTreeNode( const strus::IndexRange& range_)
		:range(range_),chld(){}
	RangeTreeNode( const RangeTreeNode& o)
		:range(o.range),chld(o.chld){}

	void add( const RangeTreeNode& nd)
		{chld.push_back(nd);}
};

static void checkRange( const char* title, const strus::IndexRange& val)
{
	if (val.start() >= val.end())
	{
		throw strus::runtime_error("bad random structures (%s) in test document created", title);
	}
}
static void checkOverlap( const strus::IndexRange& val1, const strus::IndexRange& val2)
{
	if (val1.overlap( val2))
	{
		throw strus::runtime_error("overlapping structures in test document created on the same level");
	}
}

static int countNodes( const RangeTreeNode& tree)
{
	int rt = 1;
	std::list<RangeTreeNode>::const_iterator ci = tree.chld.begin(), ce = tree.chld.end();
	for (; ci != ce; ++ci)
	{
		rt += countNodes( *ci);
	}
	return rt;
}

static RangeTreeNode createRandomTree( int nofChilds, int depth, const strus::IndexRange& cover)
{
	RangeTreeNode rt( cover);
	int mi = 0, me = g_random.get( 0, nofChilds);
	strus::Index start = cover.start();
	strus::Index diff = 1 + (cover.end() - cover.start()) / (me + 1);
	start += g_random.get( 0, (int)std::sqrt(diff)+1);

	for (; mi != me && start < cover.end(); ++mi)
	{
		strus::Index end = start + g_random.get( 0, g_random.get(0, 2*diff)) + 1;
		if (end > cover.end()) break;
		strus::IndexRange field( start, end);
		if (rt.chld.empty())
		{
			if (field == cover) return rt;
			//... avoid this kind of structure as it leads to overlapping of 
			//	structures of different nodes. Having that we cannot join
			//	contents with same header in the tree search without huge effort.
		}
		else
		{
			checkOverlap( rt.chld.back().range, field);
		}
		if (depth <= 0)
		{
			rt.add( field);
		}
		else
		{
			rt.add( createRandomTree( nofChilds, depth-1, field));
		}
		checkRange( "create tree node", rt.chld.back().range);
		start = end + g_random.get( 0, g_random.get( 0, (int)std::sqrt(diff)+1));
	}
	return rt;
}


static void createTreeNodeStructures( std::vector<strus::test::StructureDef>& result, const RangeTreeNode& node)
{
	if (node.chld.empty()) return;

	std::list<RangeTreeNode>::const_iterator ci = node.chld.begin(), ce = node.chld.end();
	strus::IndexRange header = ci->range;
	createTreeNodeStructures( result, *ci);
	for (++ci; ci != ce; ++ci)
	{
		checkRange( "add result content", ci->range);
		checkRange( "add result header", header);
		result.push_back( strus::test::StructureDef( g_structureName, header, ci->range));
		createTreeNodeStructures( result, *ci);
	}
}

static std::pair<strus::IndexRange,strus::IndexRange>
	findAnswerStructure_( const RangeTreeNode& node, strus::Index headerpos, strus::Index contentpos)
{
	std::pair<strus::IndexRange,strus::IndexRange> rt;
	if (node.chld.empty()) return std::pair<strus::IndexRange,strus::IndexRange>();
	{
		std::list<RangeTreeNode>::const_iterator
			ci = node.chld.begin(), ce = node.chld.end();
		if (headerpos >= node.range.end()) return rt;
	
		for (; ci != ce; ++ci)
		{
			rt = findAnswerStructure_( *ci, headerpos, contentpos);
			if (rt.first.defined()) break;
		}
	}{
		std::list<RangeTreeNode>::const_iterator
			ci = node.chld.begin(), ce = node.chld.end();
		if (headerpos < ci->range.end())
		{
			if (rt.first.contain( headerpos)) return rt;
			strus::IndexRange firstNode = ci->range;
			++ci;
			if (ci != ce)
			{
				rt.first = firstNode;
			}
			if (contentpos < node.range.end())
			{
				for (; ci != ce; ++ci)
				{
					if (contentpos < ci->range.end())
					{
						rt.second = ci->range;
						break;
					}
				}
			}
		}
	}
	return rt;
}

static std::pair<strus::IndexRange,strus::IndexRange>
	findAnswerStructure( const RangeTreeNode& node, strus::Index headerpos, strus::Index contentpos)
{
	return findAnswerStructure_( node, headerpos, contentpos);
}

static std::pair<RangeTreeNode,std::vector<strus::test::StructureDef> >
	createRandomStructures( int nofStructures)
{
	enum {DepthDistributionSize=30};
	static const int depthDistribution[ DepthDistributionSize] = {0,1,1,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,5,5,5,6,6,7};

	std::pair<RangeTreeNode,std::vector<strus::test::StructureDef> > rt;
	enum {NofDist=4};
	static const int dist[ NofDist] = {1,0xFF,0xfFF,0xffFF};
	int depth = depthDistribution[ g_random.get( 0, DepthDistributionSize)];
	float nofChilds = (depth == 0) ? nofStructures : std::pow( (double)nofStructures, 1.0/(double)depth);

	strus::Index start=0, end=0;
	while (start >= end)
	{
		start = g_random.get( 0, 5) == 1 ? 1 : g_random.get( 1, dist[ g_random.get( 0, NofDist)]);
		end = g_random.get( 0, 5) == 1 ? 0xffFF : g_random.get( 1, dist[ g_random.get( 0, NofDist)]);
	}
	strus::IndexRange cover( start, end);

	rt.first = createRandomTree( nofChilds+0.5, depth, cover);
	if (g_verbosity >= 1) std::cerr << strus::string_format( "create random tree with depth %d and %d nodes", depth, countNodes( rt.first)) << std::endl;
	createTreeNodeStructures( rt.second, rt.first);
	std::vector<strus::test::StructureDef>::const_iterator
		ri = rt.second.begin(), re = rt.second.end();
	for (; ri != re; ++ri)
	{
		checkRange( "result structure content", ri->content());
		checkRange( "result structure header", ri->header());
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
	RangeTreeNode structuretree;
	std::vector<strus::test::StructureDef> structurelist;

	Document( int didx, const RangeTreeNode& structuretree_, const std::vector<strus::test::StructureDef>& structurelist_)
		:docid(strus::string_format("D%d", didx)),structuretree(structuretree_),structurelist(structurelist_){}
	Document( const Document& o)
		:docid(o.docid),structuretree(o.structuretree),structurelist(o.structurelist){}

	static Document createRandom( int didx, int nofStructures)
	{
		std::pair<RangeTreeNode,std::vector<strus::test::StructureDef> >
			stu = createRandomStructures( nofStructures);
		return Document( didx+1, stu.first, stu.second);
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

	Collection( int nofDocuments, int nofStructures)
		:doclist()
	{
		int di = 0, de = nofDocuments;
		for (; di != de; ++di)
		{
			doclist.push_back( Document::createRandom( di, nofStructures));
			if (g_verbosity >= 2)
			{
				std::cerr << "structures created for " << doclist.back().docid << std::endl;
				std::vector<strus::test::StructureDef>::const_iterator
					si = doclist.back().structurelist.begin(),
					se = doclist.back().structurelist.end();
				for (; si != se; ++si)
				{
					std::cerr << si->name() << strus::string_format(": [%d,%d] -> [%d,%d]", (int)si->header().start(), (int)si->header().end(), (int)si->content().start(), (int)si->content().end()) << std::endl;
				}
				std::cerr << std::endl;
			}
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

					buildDocument( doc.get(), di->structurelist);
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

				buildDocument( doc.get(), di->structurelist);
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

		Answer()
			:header(),content(){}
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
		if (doc.structurelist.empty())
		{
			return QueryAnswerPair( Query( docid, 0, 0), Answer());
		}
		else
		{
			std::size_t stuidx = g_random.get( 0, doc.structurelist.size());
			const strus::test::StructureDef& studef = doc.structurelist[ stuidx];
			strus::IndexRange hdr = studef.header();
			strus::IndexRange mbr = studef.content();
	
			strus::Index headerpos = randomIndex( hdr);
			strus::Index contentpos = randomIndex( mbr);
			std::pair<strus::IndexRange,strus::IndexRange>
				answer = findAnswerStructure( doc.structuretree, headerpos, contentpos);
			strus::IndexRange answer_hdr = answer.first;
			strus::IndexRange answer_mbr = answer.second;
	
			return QueryAnswerPair( Query( docid, headerpos, contentpos), Answer( answer_hdr, answer_mbr));
		}
	}
};

static std::pair<strus::IndexRange,int> findQuerySource( strus::StructureIteratorInterface* sitr, strus::Index structno, strus::Index pos)
{
	std::pair<strus::IndexRange,int> rt;
	int li = 0, le = sitr->levels();
	for (; li != le; ++li)
	{
		strus::IndexRange source = sitr->skipPos( li, pos);
		for (;source.defined(); source = sitr->skipPos( li, source.end()))
		{
			strus::StructureLinkArray lnkar = sitr->links( li);
			int si = 0, se = lnkar.nofLinks();
			for (; si != se; ++si)
			{
				const strus::StructureLink& lnk = lnkar[ si];
				if (lnk.header() == true && lnk.structno() == structno)
				{
					if (!rt.first.defined() || (source.defined() && rt.first.start() < source.start() && pos >= source.start()))
					{
						rt.first = source;
						rt.second = lnk.index();
						break;
					}
					if (source.defined())
					{
						if (source.contain( pos))
						{
							rt.first = source;
							rt.second = lnk.index();
							break;
						}
						if (pos < source.start() && pos < rt.first.start() && source.start() < rt.first.start())
						{
							rt.first = source;
							rt.second = lnk.index();
							break;
						}
					}
				}
			}
			if (si != se) break;
		}
	}
	return rt;
}

static strus::IndexRange findQuerySink( strus::StructureIteratorInterface* sitr, strus::Index structno, int structidx, strus::Index pos)
{
	strus::IndexRange rt;
	int li = 0, le = sitr->levels();
	for (; li != le; ++li)
	{
		strus::IndexRange sink = sitr->skipPos( li, pos);
		bool valid_candidate_found = false;
		while (!valid_candidate_found && sink.defined())
		{
			strus::StructureLinkArray lnkar = sitr->links( li);
			int si = 0, se = lnkar.nofLinks();
			for (; si != se; ++si)
			{
				const strus::StructureLink& lnk = lnkar[ si];
				if (lnk.header() == false && lnk.structno() == structno && lnk.index() == structidx)
				{
					rt = sink;
					valid_candidate_found = true;
					break;
				}
			}
			sink = sitr->skipPos( li, sink.end());
		}
	}
	return rt;
}

static void verifyQueryAnswer( Storage& storage, strus::StructureIteratorInterface* sitr, const Collection::QueryAnswerPair& qa)
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

static void testLargeStructures( int nofDocuments, int nofStructures, int commitSize, int nofQueryies, const std::string& selectDocid)
{
	Storage storage( g_fileLocator, g_errorhnd);
	storage.open( "path=storage", true);
	Collection collection( nofDocuments, nofStructures);

	if (selectDocid.empty())
	{
		collection.insert( storage.sci.get(), commitSize);
	}
	else
	{
		collection.insert( storage.sci.get(), selectDocid);
	}
	if (g_dumpCollection) collection.dump( std::cout);

	int qi = 0, qe = nofQueryies;
	strus::local_ptr<strus::StructureIteratorInterface> sitr( storage.sci->createStructureIterator());
	if (!sitr.get()) throw std::runtime_error("failed to create structure iterator");

	if (g_verbosity >= 1) {fprintf( stderr, "\n"); fflush( stderr);}
	try
	{
		int qmodta = std::sqrt( nofQueryies / 10);
		int qmod = 1;
		while (qmod < qmodta) qmod *= 10;

		for (; qi != qe; ++qi)
		{
			if (qi && qi % qmod == 0)
			{
				if (g_verbosity >= 1) fprintf( stderr, "\rissued %d queries", qi);
			}
			Collection::QueryAnswerPair qa = collection.randomQueryAnswer();
			if (selectDocid.empty() || qa.query.docid == selectDocid)
			{
				verifyQueryAnswer( storage, sitr.get(), qa);
			}
		}
	}
	catch (const std::runtime_error& err)
	{
		if (g_verbosity >= 1) {fprintf( stderr, "\rissued %d queries\n", qi); fflush( stderr);}
		throw std::runtime_error( err.what());
	}
	if (g_verbosity >= 1) {fprintf( stderr, "\rissued %d queries\n", qi); fflush( stderr);}
}

static void printUsage()
{
	std::cerr << "usage: testLargeStructures [options] <nofdocs> <nofstu> <commitsize> <nofqry>" << std::endl;
	std::cerr << "description: Inserts a collection of documents with large structures" << std::endl;
	std::cerr << "             and verify the stored data." << std::endl;
	std::cerr << "options:" << std::endl;
	std::cerr << "  -h             :print usage" << std::endl;
	std::cerr << "  -V             :increment verbosity (repeating)" << std::endl;
	std::cerr << "  -D             :dump collection created" << std::endl;
	std::cerr << "  -K             :keep artefacts, do not clean up" << std::endl;
	std::cerr << "  -W <seed>      :sepecify pseudo random number generator seed (int)" << std::endl;
	std::cerr << "  -F <docid>     :only process insert of document with id <docid>" << std::endl;
	std::cerr << "<nofdocs>        :number of documents inserted" << std::endl;
	std::cerr << "<nofstu>         :average number of structures per document" << std::endl;
	std::cerr << "<commitsize>     :number of documents inserted per transaction" << std::endl;
	std::cerr << "<nofqry>         :number of random queries to verify stored structures" << std::endl;
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
		if (argi + 4 > argc)
		{
			printUsage();
			throw std::runtime_error( "too few arguments");
		}
		if (argi + 4 < argc)
		{
			printUsage();
			throw std::runtime_error( "too many arguments");
		}
		int ai = 0;
		int nofDocuments = strus::numstring_conv::toint( argv[ argi+ai], std::strlen(argv[ argi+ai]), std::numeric_limits<int>::max());
		++ai;
		int nofStructures = strus::numstring_conv::toint( argv[ argi+ai], std::strlen(argv[ argi+ai]), std::numeric_limits<int>::max());
		++ai;
		int commitSize = strus::numstring_conv::toint( argv[ argi+ai], std::strlen(argv[ argi+ai]), std::numeric_limits<int>::max());
		++ai;
		int nofQueryies = strus::numstring_conv::toint( argv[ argi+ai], std::strlen(argv[ argi+ai]), std::numeric_limits<int>::max());
		++ai;

		testLargeStructures( nofDocuments, nofStructures, commitSize, nofQueryies, selectDocid);
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


