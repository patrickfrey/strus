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
#include "strus/lib/queryeval.hpp"
#include "strus/lib/queryproc.hpp"
#include "strus/base/fileio.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/numstring.hpp"
#include "strus/base/local_ptr.hpp"
#include "strus/base/shared_ptr.hpp"
#include "strus/base/pseudoRandom.hpp"
#include "strus/base/stdint.h"
#include "strus/base/math.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/fileLocatorInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/storageTransactionInterface.hpp"
#include "strus/storageDocumentInterface.hpp"
#include "strus/storageDocumentUpdateInterface.hpp"
#include "strus/storageDumpInterface.hpp"
#include "strus/structureIteratorInterface.hpp"
#include "strus/queryInterface.hpp"
#include "strus/queryEvalInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/summarizerFunctionInstanceInterface.hpp"
#include "strus/storage/weightedField.hpp"
#include "strus/storage/weightedDocument.hpp"
#include "strus/storage/resultDocument.hpp"
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

	void printToString( std::string& result, int indent) const
	{
		result.append( std::string( 2*indent, ' '));
		result.append( strus::string_format( "[%d,%d]", (int)field.start(), (int)field.end()));
		std::vector<int>::const_iterator fi = featar.begin(), fe = featar.end();
		for (; fi != fe; ++fi)
		{
			result.append( strus::string_format(" %d", *fi));
		}
		result.append("\n");
		std::list<TitleTreeNode>::const_iterator ci = chld.begin(), ce = chld.end();
		for (; ci != ce; ++ci)
		{
			ci->printToString( result, indent+1);
		}
	}
	std::string tostring() const
	{
		std::string rt;
		printToString( rt, 0);
		return rt;
	}
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
	std::vector<int> features;

	Query( const std::vector<int>& features_)
		:features(features_){}
	Query( const Query& o)
		:features(o.features){}

	static void shuffle( std::vector<std::vector<int> >& ar)
	{
		int ii=0, nn=ar.size();
		for (; ii<nn; ++ii)
		{
			int ai = g_random.get( 0, ar.size());
			int bi = g_random.get( 0, ar.size());
			if (ai != bi) ar[ ai].swap( ar[ bi]);
		}
	}

	static std::vector<int> serFeatureList( const std::vector<std::vector<int> >& ar)
	{
		std::vector<int> rt;
		std::vector<std::vector<int> >::const_iterator ai = ar.begin(), ae = ar.end();
		for (; ai != ae; ++ai)
		{
			rt.insert( rt.end(), ai->begin(), ai->end());
		}
		return rt;
	}

	static Query createRandomQuery( const TitleTreeNode& tree)
	{
		std::vector<std::vector<int> > features_;

		std::vector<const TitleTreeNode*> completeNodes;
		completeNodes.push_back( &tree);
		while (g_random.get( 0, 1+completeNodes.size()) == 0)
		{
			if (completeNodes.back()->chld.empty()) break;
			int cidx = g_random.get( 0, completeNodes.back()->chld.size());
			std::list<TitleTreeNode>::const_iterator
				ci = completeNodes.back()->chld.begin(),
				ce = completeNodes.back()->chld.end();

			for (; cidx > 0 && ci != ce; --cidx,++ci){}
			bool leaveOutNode = g_random.get( 0, completeNodes.size()) == 1;
			// ... first node never left out, because this call always returns 0 for the first node
			if (!leaveOutNode) completeNodes.push_back( &*ci);
		}
		const TitleTreeNode* finalNode = completeNodes.back();
		completeNodes.pop_back();

		std::vector<const TitleTreeNode*>::const_iterator
			si = completeNodes.begin(), se = completeNodes.end();
		for (; si != se; ++si)
		{
			features_.push_back( std::vector<int>());
			std::vector<int>::const_iterator fi = (*si)->featar.begin(), fe = (*si)->featar.end();
			for (; fi != fe; ++fi)
			{
				features_.back().push_back( *fi);
			}
		}
		features_.push_back( std::vector<int>());
		int nofFinal = g_random.get( 1, finalNode->featar.size());
		std::vector<int>::const_iterator fi = finalNode->featar.begin(), fe = finalNode->featar.end();
		for (; fi != fe && nofFinal > 0; ++fi,--nofFinal)
		{
			features_.back().push_back( *fi);
		}
		shuffle( features_);
		return Query( serFeatureList( features_));
	}

	static bool subMatch( std::vector<int>::const_iterator fi, const std::vector<int>::const_iterator fe,
				std::vector<int>::const_iterator ai, const std::vector<int>::const_iterator ae)
	{
		for (; fi != fe && ai != ae && *ai == *fi; ++fi,++ai){}
		return ai == ae;
	}

	static std::vector<int> maskSubMatches( const std::vector<int>& fa, std::vector<int>::const_iterator fi, const std::vector<int>::const_iterator fe)
	{
		std::vector<int> rt = fa;
		std::size_t rpos = fi - fa.begin();
		if (rpos >= rt.size()) throw std::logic_error( "logic error (maskSubMatches)");
		std::vector<int>::iterator ri = rt.begin() + rpos;
		for (; fi != fe; ++fi,++ri) {*ri = -1;}
		return rt;
	}
	static bool occupy( const std::vector<int>& fa, std::vector<const TitleTreeNode*>::const_iterator ti, const std::vector<const TitleTreeNode*>::const_iterator te)
	{
		if (ti == te)
		{
			std::vector<int>::const_iterator fi = fa.begin(), fe = fa.end();
			for (; fi != fe; ++fi)
			{
				if (*fi != -1) return false;
			}
			return true;
		}
		else
		{
			if (NULL == (*ti))
			{
				return occupy( fa, ti+1, te);
			}
			else if (ti+1 == te)
			{
				std::vector<int>::const_iterator fi = fa.begin(), fe = fa.end();
				for (; fi != fe; ++fi)
				{
					if (*fi >= 0)
					{
						std::vector<int>::const_iterator fn = fi;
						for (; fn != fe && *fn >= 0; ++fn){}
						std::size_t nn = fn - fi;
						if (nn > (*ti)->featar.size()) nn = (*ti)->featar.size();
						if (subMatch( fi, fn, (*ti)->featar.begin(), (*ti)->featar.begin() + nn))
						{
							std::vector<int> fa_new = maskSubMatches( fa, fi, fi + nn);
							if (occupy( fa_new, ti+1, te)) return true;
						}
					}
				}
			}
			else
			{
				std::vector<int>::const_iterator fi = fa.begin(), fe = fa.end();
				for (; fi != fe; ++fi)
				{
					int nn = (*ti)->featar.size();
					if (*fi >= 0 && subMatch( fi, fe, (*ti)->featar.begin(), (*ti)->featar.end()))
					{
						std::vector<int> fa_new = maskSubMatches( fa, fi, fi + nn);
						if (occupy( fa_new, ti+1, te)) return true;
					}
				}
			}
			return false;
		}
	}

	strus::WeightedField verifyAnswer( const std::vector<const TitleTreeNode*>& nodestk) const
	{
		int nn = 0;
		std::vector<const TitleTreeNode*>::const_iterator ni = nodestk.begin(), ne = nodestk.end();
		for (; ni != ne; ++ni) {nn += (*ni)->featar.size();}
		int missingFeatures = nn - (int)features.size();
		if (missingFeatures < 0) return strus::WeightedField();
	}

	int minFeatureCount( const std::vector<const TitleTreeNode*>& nodestk) const
	{
		std::vector<const TitleTreeNode*>::const_iterator ni = nodestk.begin(), ne = nodestk.end();

		int rt = 1;
		for (--ne; ni != ne; ++ni)
		{
			rt += (*ni == NULL) ? 0 : (*ni)->featar.size();
		}
		return rt;
	}

	void findAnswers( double hierarchyWeightFactor, std::vector<strus::WeightedField>& res, const std::vector<const TitleTreeNode*>& nodestk) const
	{
		int mc = minFeatureCount( nodestk);
		if (mc > (int)features.size()) return;
		if (occupy( features, nodestk.begin(), nodestk.end()))
		{
			// ... features can be covered by nodes in the tree
			// Find out how many features are used per node.
			// This determines the weight without hierarchy factor:
			std::vector<const TitleTreeNode*>::const_iterator ni = nodestk.begin(), ne = nodestk.end();
			std::vector<int> numberOfFeaturesUsedPerNode;
			int fno = 0;
			for (; ni != ne; ++ni)
			{
				int uu = (*ni == NULL) ? 0 : (*ni)->featar.size();
				fno += uu;
				if (fno > (int)features.size())
				{
					uu -= fno - (int)features.size();
					if (uu <= 0) throw std::logic_error( "logic error (findAnswers)");
				}
				numberOfFeaturesUsedPerNode.push_back( uu);
			}
			// Calculate the weight of the solution as sum of the node weights 
			//	multiplied by the hierarchy factor:
			strus::Index res_start = nodestk.back()->field.start();
			strus::Index res_end = res_start + nodestk.back()->featar.size();
			strus::IndexRange field( res_start, res_end);
			double ww = 0.0;
			double factor = 1.0;
			std::vector<int>::const_iterator
				ui = numberOfFeaturesUsedPerNode.begin(),
				ue = numberOfFeaturesUsedPerNode.end();
			for (; ui != ue; ++ui)
			{
				ww += factor * ((double)*ui / (double)features.size());
				factor *= hierarchyWeightFactor;
			}
			// Add final result:
			res.push_back( strus::WeightedField( field, ww));
		}
		if (!nodestk.empty())
		{
			// Create follow candidates for a solution:
			std::vector<const TitleTreeNode*> new_nodestk( nodestk);
			std::list<TitleTreeNode>::const_iterator
				ci = nodestk.back()->chld.begin(), ce = nodestk.back()->chld.end();
			for (; ci != ce; ++ci)
			{
				new_nodestk.push_back( &*ci);
				findAnswers( hierarchyWeightFactor, res, new_nodestk);
				new_nodestk.pop_back();
			}
			if (new_nodestk.size() > 1)
			{
				// Include candidate with a dismissed node that is not the root node
				//	(condition new_nodestk.size() > 1):
				new_nodestk.back() = NULL;
				ci = nodestk.back()->chld.begin();
				for (; ci != ce; ++ci)
				{
					new_nodestk.push_back( &*ci);
					findAnswers( hierarchyWeightFactor, res, new_nodestk);
					new_nodestk.pop_back();
				}
			}
		}
	}

	std::vector<strus::WeightedField> answer( double hierarchyWeightFactor, const TitleTreeNode& tree) const
	{
		std::vector<strus::WeightedField> rt;
		std::vector<const TitleTreeNode*> stk;
		stk.push_back( &tree);
		findAnswers( hierarchyWeightFactor, rt, stk);
		return rt;
	}

	std::string tostring() const
	{
		std::string rt;
		std::vector<int>::const_iterator fi = features.begin(), fe = features.end();
		for (; fi != fe; ++fi)
		{
			if (!rt.empty()) rt.push_back( ' ');
			rt.append( strus::string_format("%d", *fi));
		}
		return rt;
	}
};

static void printStructureList( std::ostream& out, std::vector<strus::test::StructureDef>& structurelist)
{
	std::vector<strus::test::StructureDef>::const_iterator
		si = structurelist.begin(), se = structurelist.end();
	for (; si != se; ++si)
	{
		out << strus::string_format("%s [%d,%d] => [%d,%d]",
				si->name().c_str(), (int)si->header().start(), (int)si->header().end(),
				(int)si->content().start(), (int)si->content().end()) << std::endl;
	}
}

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

struct LabeledTitle
{
	std::string text;
	int hierarchy;

	LabeledTitle( const std::string& text_, int hierarchy_)
		:text(text_),hierarchy(hierarchy_){}
	LabeledTitle( const LabeledTitle& o)
		:text(o.text),hierarchy(o.hierarchy){}
};

static void fillTitles( std::vector<LabeledTitle>& titlelist, const TitleTreeNode& node, const strus::IndexRange& field, int depth)
{
	if (node.field.cover( field))
	{
		std::vector<int>::const_iterator fi = node.featar.begin(), fe = node.featar.end();
		std::string titletext;
		for (; fi != fe; ++fi)
		{
			if (!titletext.empty()) titletext.push_back(' ');
			titletext.append( strus::string_format( "F%d", *fi));
		}
		if (!titletext.empty()) titlelist.push_back( LabeledTitle( titletext, depth));
		std::list<TitleTreeNode>::const_iterator ci = node.chld.begin(), ce = node.chld.end();
		for (; ci != ce; ++ci)
		{
			fillTitles( titlelist, *ci, field, depth+1);
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
	std::vector<TitleTreeNode> titletreelist;
	std::vector<strus::test::StructureDef> structurelist;
	std::vector<strus::test::Feature> featurelist;

	Document()
		:docid(),titletreelist(),structurelist(),featurelist(){}
	Document( int didx, const std::vector<TitleTreeNode>& titletreelist_)
		:docid(strus::string_format("D%d", didx)),titletreelist(titletreelist_)
		,structurelist(),featurelist()
	{
		std::vector<TitleTreeNode>::const_iterator ti = titletreelist.begin(), te = titletreelist.end();
		for (; ti != te; ++ti)
		{
			fillStructureList( structurelist, *ti);
			fillFeatureList( featurelist, *ti);
		}
	}
	Document( const Document& o)
		:docid(o.docid),titletreelist(o.titletreelist)
		,structurelist(o.structurelist),featurelist(o.featurelist){}

	bool defined()
	{
		return !docid.empty();
	}

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

		std::vector<TitleTreeNode> titletreelist_;
		int startpos = 0;
		do
		{
			TitleTreeNode titletree_ = createTitleTree( nofChilds, nofTerms, nofFeatures, depth, startpos);
			startpos = titletree_.field.end();
			titletreelist_.push_back( titletree_);
		} while (g_random.get( 0, 3) == 1);
		return Document( didx+1, titletreelist_);
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
		doc->setAttribute( "docid", docid);
		std::vector<strus::test::StructureDef>::const_iterator
			xi = structurelist.begin(), xe = structurelist.end();
		for (; xi != xe; ++xi)
		{
			doc->addSearchIndexStructure( xi->name(), xi->header(), xi->content());
		}
		doc->done();
	}

	std::string tostring() const
	{
		std::string rt;
		std::vector<TitleTreeNode>::const_iterator
			ti = titletreelist.begin(), te = titletreelist.end();
		for (; ti != te; ++ti)
		{
			ti->printToString( rt, 0);
		}
		return rt;
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
			Document data = getDocument( selectDocid);
			strus::local_ptr<strus::StorageDocumentInterface>
				doc( transaction->createDocument( selectDocid));
			if (!doc.get()) throw strus::runtime_error("error creating document to insert");

			data.build( doc.get());
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
				if (g_verbosity >= 2)
				{
					std::cerr << "insert " << di->tostring() << std::endl;
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

	Document getDocument( const std::string& docid)
	{
		std::vector<Document>::const_iterator di = doclist.begin(), de = doclist.end();
		for (; di != de; ++di)
		{
			if (di->docid == docid) return *di;
		}
		throw std::runtime_error( strus::string_format("document id '%s' not defined", docid.c_str()));
	}

	Query randomQuery() const
	{
		const Document& doc = doclist[ g_random.get( 0, doclist.size())];
		const TitleTreeNode& tree = doc.titletreelist[ g_random.get( 0, doc.titletreelist.size())];
		return Query::createRandomQuery( tree);
	}

	std::vector<Query> randomQueries( int nofQueryies) const
	{
		std::vector<Query> rt;
		int qi = 0, qe = nofQueryies;
		for (; qi != qe; ++qi)
		{
			rt.push_back( randomQuery());
		}
		return rt;
	}

	strus::QueryResult expectedResult( double hierarchyWeightFactor, int maxNofRanks, const Query& query, strus::StorageClientInterface* storage)
	{
		std::vector<strus::ResultDocument> ranks;
		std::vector<Document>::const_iterator di = doclist.begin(), de = doclist.end();
		for (; di != de; ++di)
		{
			strus::Index docno = storage->documentNumber( di->docid);
			if (docno)
			{
				std::vector<TitleTreeNode>::const_iterator
					ti = di->titletreelist.begin(), te = di->titletreelist.end();
				for (; ti != te; ++ti)
				{
					std::vector<strus::WeightedField> answer = query.answer( hierarchyWeightFactor, *ti);
					std::vector<strus::WeightedField>::const_iterator
						ai = answer.begin(), ae = answer.end();
					for (; ai != ae; ++ai)
					{
						strus::WeightedDocument wdoc( docno, *ai);
						std::vector<strus::SummaryElement> summary;
						summary.push_back( strus::SummaryElement( "docid", di->docid, 1.0));

						std::vector<LabeledTitle> titlelist;
						fillTitles( titlelist, *ti, ai->field(), 0/*depth*/);
						std::vector<LabeledTitle>::const_iterator
							li = titlelist.begin(), le = titlelist.end();
						for (int lidx=0; li != le; ++li,++lidx)
						{
							summary.push_back( strus::SummaryElement( "header", li->text, 1.0, li->hierarchy));
						}
						ranks.push_back( strus::ResultDocument( wdoc, summary));
					}
				}
			}
		}
		int nofRanked = ranks.size();
		if (maxNofRanks <= 0 || maxNofRanks > (int)ranks.size())
		{
			std::sort( ranks.begin(), ranks.end(), std::greater<strus::WeightedDocument>());
		}
		else
		{
			std::nth_element( ranks.begin(), ranks.begin()+maxNofRanks, ranks.end(), std::greater<strus::WeightedDocument>());
			ranks.resize( maxNofRanks);
			std::sort( ranks.begin(), ranks.end(), std::greater<strus::WeightedDocument>());
		}
		return strus::QueryResult( 0/*evaluationPass*/, nofRanked, nofRanked/*nofVisited*/, ranks);
	}

	std::map<strus::Index,std::string> docnoDocidMap( const strus::StorageClientInterface* storage) const
	{
		std::map<strus::Index,std::string> rt;
		int didx = 0;
		for (; didx < (int)doclist.size(); ++didx)
		{
			std::string docid = strus::string_format( "D%d", didx+1);
			strus::Index docno = storage->documentNumber( docid);
			if (!docno) throw std::runtime_error( strus::string_format("logic error: document undefined %s", docid.c_str()));
			rt[ docno] = docid;
		}
		return rt;
	}

	std::map<strus::Index,std::string> docnoDocidMap( const strus::StorageClientInterface* storage, const std::string& selectDocid) const
	{
		std::map<strus::Index,std::string> rt;
		strus::Index docno = storage->documentNumber( selectDocid);
		if (!docno) throw std::runtime_error( strus::string_format("logic error: document undefined %s", selectDocid.c_str()));
		rt[ docno] = selectDocid;
		return rt;
	}
};

static strus::QueryResult evaluateQuery( strus::QueryEvalInterface* qeval, strus::StorageClientInterface* storage, const Query& query, int maxNofRanks)
{
	strus::Reference<strus::QueryInterface> qry( qeval->createQuery( storage));
	if (!qry.get()) throw std::runtime_error( g_errorhnd->fetchError());
	std::vector<int>::const_iterator fi = query.features.begin(), fe = query.features.end();
	for (; fi != fe; ++fi)
	{
		std::string word = strus::string_format( "f%d", *fi);
		qry->pushTerm( "word", word, 1);
		qry->defineFeature( "search");
	}
	return qry->evaluate( 0, maxNofRanks);
}

static bool compareResultAgainstExpected( const strus::ResultDocument& result, const strus::WeightedDocument& expected)
{
	if (result.docno() != expected.docno()) return false;
	if (result.field() != expected.field()) return false;
	if (strus::Math::abs( result.weight() - expected.weight()) > std::numeric_limits<float>::epsilon()) return false;
	return true;
}

static std::string errorMessageResultNotFound(
		const std::map<strus::Index,std::string>& docnoDocidMap, const std::string& testdescr,
		const strus::ResultDocument& result)
{
	std::map<strus::Index,std::string>::const_iterator di = docnoDocidMap.find( result.docno());
	if (di == docnoDocidMap.end()) throw std::runtime_error("undefined document number");

	return strus::string_format( "result document %s (%d) field [%d,%d] weight %.5f unexpected match in %s",
					di->second.c_str(), (int)result.docno(),
					(int)result.field().start(), (int)result.field().end(),
					result.weight(), testdescr.c_str());
}

static std::string errorMessageExpectedNotFound(
		const std::map<strus::Index,std::string>& docnoDocidMap, const std::string& testdescr,
		const strus::WeightedDocument& expected)
{
	std::map<strus::Index,std::string>::const_iterator di = docnoDocidMap.find( expected.docno());
	if (di == docnoDocidMap.end()) throw std::runtime_error("undefined document number");

	return strus::string_format( "expected document %s (%d) field [%d,%d] weight %.5f not found in %s, ",
					di->second.c_str(), (int)expected.docno(), 
					(int)expected.field().start(), (int)expected.field().end(),
					expected.weight(), testdescr.c_str());
}

bool testResultSummaryAgainstExpected( const std::vector<strus::SummaryElement>& result, const std::vector<strus::SummaryElement>& expected)
{
	if (result.size() != expected.size()) return false;
	std::vector<strus::SummaryElement>::const_iterator
		ri = result.begin(), re = result.end();
	std::vector<strus::SummaryElement>::const_iterator
		ei = expected.begin(), ee = expected.end();
	for (; ei != ee && ri != re; ++ri,++ei)
	{
		if (ri->name() != ei->name()) return false;
		if (ri->value() != ei->value()) return false;
		if (!strus::Math::isequal( ri->weight(), ei->weight())) return false;
		if (ri->index() != ei->index()) return false;
	}
	return ei == ee && ri == re;
}

static void testResultAgainstExpected( const std::map<strus::Index,std::string>& docnoDocidMap, const std::string& testdescr, const strus::QueryResult& result, const strus::QueryResult& expected, int ranksChecked)
{
	{
		std::vector<strus::ResultDocument>::const_iterator
			ri = result.ranks().begin(), re = result.ranks().end();
		int ridx=0;
		for (; ri != re && ridx < ranksChecked; ++ri,++ridx)
		{
			std::vector<strus::ResultDocument>::const_iterator
				ei = expected.ranks().begin(), ee = expected.ranks().end();
			for (; ei != ee && !compareResultAgainstExpected( *ri, *ei); ++ei){}
			if (ei == ee)
			{
				std::string errormsg
					= errorMessageResultNotFound( docnoDocidMap, testdescr, *ri);
				throw std::runtime_error( errormsg);
			}
			else
			{
				if (!testResultSummaryAgainstExpected( ri->summaryElements(), ei->summaryElements()))
				{
					throw std::runtime_error(_TXT("summary elements of result do not match expected"));
				}
			}
		}
	}{
		std::vector<strus::ResultDocument>::const_iterator
			ei = expected.ranks().begin(), ee = expected.ranks().end();
		int eidx=0;
		for (; ei != ee && eidx < ranksChecked; ++ei,++eidx)
		{
			std::vector<strus::ResultDocument>::const_iterator
				ri = result.ranks().begin(), re = result.ranks().end();
			for (; ri != re && !compareResultAgainstExpected( *ri, *ei); ++ri){}
			if (ri == re)
			{
				std::string errormsg
					= errorMessageExpectedNotFound( docnoDocidMap, testdescr, *ei);
				throw std::runtime_error( errormsg);
			}
			else
			{
				if (!testResultSummaryAgainstExpected( ri->summaryElements(), ei->summaryElements()))
				{
					throw std::runtime_error(_TXT("summary elements of result do not match expected"));
				}
			}
		}
	}
}

static void checkResult( const std::map<strus::Index,std::string>& docnoDocidMap, int qryidx, const Query& qry, const strus::QueryResult& result, const strus::QueryResult& expected, int ranksChecked)
{
	std::string qrystr = qry.tostring();
	std::string testdescr = strus::string_format( "test query %d = '%s'", qryidx, qrystr.c_str());
	testResultAgainstExpected( docnoDocidMap, testdescr, result, expected, ranksChecked);
}

static void printResult( std::ostream& out, const std::map<strus::Index,std::string>& docnoDocidMap, const strus::QueryResult& result)
{
	std::vector<strus::ResultDocument>::const_iterator ri = result.ranks().begin(), re = result.ranks().end();
	for (int ridx=1; ri != re; ++ri,++ridx)
	{
		std::map<strus::Index,std::string>::const_iterator di = docnoDocidMap.find( ri->docno());
		if (di == docnoDocidMap.end()) throw std::runtime_error( strus::string_format( "unknown document number in result %d", ri->docno()));
		out << strus::string_format( "rank %d: %s [%d,%d] %.5f",
				ridx, di->second.c_str(),
				(int)ri->field().start(), (int)ri->field().end(),
				ri->weight())
			<< std::endl;

		std::vector<strus::SummaryElement>::const_iterator
			si = ri->summaryElements().begin(),
			se = ri->summaryElements().end();
		for (; si != se; ++si)
		{
			if (si->index() >= 0)
			{
				out << strus::string_format( "\t> %s = '%s' %.5f %d",
						si->name().c_str(), si->value().c_str(), si->weight(), si->index())
					<< std::endl;
			}
			else
			{
				out << strus::string_format( "\t> %s = %s %.5f",
						si->name().c_str(), si->value().c_str(), si->weight())
					<< std::endl;
			}
		}
	}
}

static void testWeightingTitle( int nofDocuments, int nofTerms, int nofNodes, int commitSize, int nofQueryies, const std::string& selectDocid, int selectQuery)
{
	Storage storage( g_fileLocator, g_errorhnd);
	storage.open( "path=storage", true);
	Collection collection( nofDocuments, nofTerms, nofNodes);

	double hierarchyWeightFactor = ((double)g_random.get( 0, 100000) / 100000.0) * 0.3 + 0.7;
	int maxNofRanks = 200;

	strus::Reference<strus::QueryEvalInterface> qeval( strus::createQueryEval( g_errorhnd));
	if (!qeval.get()) throw std::runtime_error( g_errorhnd->fetchError());
	strus::Reference<strus::QueryProcessorInterface> queryproc( strus::createQueryProcessor( g_fileLocator, g_errorhnd));
	if (!queryproc.get()) throw std::runtime_error( g_errorhnd->fetchError());

	const strus::WeightingFunctionInterface* weightingTitle = queryproc->getWeightingFunction( "title");
	if (!weightingTitle) throw std::runtime_error( "undefined weighting function 'title'");
	strus::Reference<strus::WeightingFunctionInstanceInterface> wfunc( weightingTitle->createInstance( queryproc.get()));
	if (!wfunc.get()) throw std::runtime_error( g_errorhnd->fetchError());
	wfunc->addNumericParameter( "hf", hierarchyWeightFactor);
	wfunc->addNumericParameter( "results", strus::NumericVariant::asint( maxNofRanks));

	const strus::SummarizerFunctionInterface* summarizerAttribute = queryproc->getSummarizerFunction( "attribute");
	if (!summarizerAttribute) throw std::runtime_error( "undefined summarizer function 'attribute'");
	strus::Reference<strus::SummarizerFunctionInstanceInterface> attributefunc( summarizerAttribute->createInstance( queryproc.get()));
	if (!attributefunc.get()) throw std::runtime_error( g_errorhnd->fetchError());
	attributefunc->addStringParameter( "name", "docid");

	const strus::SummarizerFunctionInterface* summarizerStructHeader = queryproc->getSummarizerFunction( "structheader");
	if (!summarizerStructHeader) throw std::runtime_error( "undefined summarizer function 'structheader'");
	strus::Reference<strus::SummarizerFunctionInstanceInterface> headerfunc( summarizerStructHeader->createInstance( queryproc.get()));
	if (!headerfunc.get()) throw std::runtime_error( g_errorhnd->fetchError());
	headerfunc->addStringParameter( "text", "orig");
	if (g_random.get(0,2)==0) headerfunc->addStringParameter( "struct", "title");

	std::vector<strus::QueryEvalInterface::FeatureParameter> fparam;
	fparam.push_back( strus::QueryEvalInterface::FeatureParameter( "match", "search"));
	std::vector<strus::QueryEvalInterface::FeatureParameter> noparam;

	qeval->addSelectionFeature( "search");
	qeval->addWeightingFunction( wfunc.release(), fparam);
	qeval->addSummarizerFunction( "docid", attributefunc.release(), noparam);
	qeval->addSummarizerFunction( "header", headerfunc.release(), noparam);

	if (g_errorhnd->hasError())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
	std::map<strus::Index,std::string> docnoDocidMap;
	if (selectDocid.empty())
	{
		collection.insert( storage.sci.get(), commitSize);
		docnoDocidMap = collection.docnoDocidMap( storage.sci.get());
	}
	else
	{
		Document data = collection.getDocument( selectDocid);
		if (g_verbosity >= 1)
		{
			std::string docstr = data.tostring();
			std::cerr << strus::string_format( "document %s:\n", selectDocid.c_str());
			std::cerr << docstr << std::endl;
			std::cerr << "structures:\n";
			printStructureList( std::cerr, data.structurelist);
			std::cerr << std::endl;
		}
		collection.insert( storage.sci.get(), selectDocid);
		docnoDocidMap = collection.docnoDocidMap( storage.sci.get(), selectDocid);
	}

	std::vector<Query> queries = collection.randomQueries( nofQueryies);
	int ranksChecked = maxNofRanks / 2;

	if (selectQuery >= 0)
	{
		if (selectQuery >= (int)queries.size())
		{
			throw std::runtime_error("selected query undefined");
		}
		const Query& qry = queries[ selectQuery];
		std::string qrystr = qry.tostring();
		if (g_verbosity)
		{
			std::cerr << strus::string_format( "selecting query %d: %s", selectQuery, qrystr.c_str()) << std::endl;
		}
		strus::QueryResult expected = collection.expectedResult( hierarchyWeightFactor, maxNofRanks, qry, storage.sci.get());
		strus::QueryResult result = evaluateQuery( qeval.get(), storage.sci.get(), qry, maxNofRanks);

		if (g_verbosity >= 1)
		{
			std::cerr << "result:" << std::endl;
			if (result.ranks().empty())
			{
				std::cerr << "(empty)" << std::endl;
			}
			else
			{
				printResult( std::cerr, docnoDocidMap, result);
			}
			std::cerr << "expected:" << std::endl;
			if (expected.ranks().empty())
			{
				std::cerr << "(empty)" << std::endl;
			}
			else
			{
				printResult( std::cerr, docnoDocidMap, expected);
			}
		}
		checkResult( docnoDocidMap, selectQuery, qry, result, expected, ranksChecked);
	}
	else
	{
		std::vector<Query>::const_iterator qi = queries.begin(), qe = queries.end();
		for (int qidx=0;qi != qe; ++qi,++qidx)
		{
			if (g_verbosity >= 2)
			{
				std::string qrystr = qi->tostring();
				std::cerr << strus::string_format( "issue query %d: %s", qidx, qrystr.c_str()) << std::endl;
			}
			strus::QueryResult expected = collection.expectedResult( hierarchyWeightFactor, maxNofRanks, *qi, storage.sci.get());
			strus::QueryResult result = evaluateQuery( qeval.get(), storage.sci.get(), *qi, maxNofRanks);

			if (g_verbosity >= 2)
			{
				std::cerr << "result:" << std::endl;
				if (result.ranks().empty())
				{
					std::cerr << "(empty)" << std::endl;
				}
				else
				{
					printResult( std::cerr, docnoDocidMap, result);
				}
				std::cerr << "expected:" << std::endl;
				if (expected.ranks().empty())
				{
					std::cerr << "(empty)" << std::endl;
				}
				else
				{
					printResult( std::cerr, docnoDocidMap, expected);
				}
			}
			checkResult( docnoDocidMap, qidx, *qi, result, expected, ranksChecked);
		}
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
	std::cerr << "  -Q <qryno>     :only process query with index <qryno>" << std::endl;
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
	int selectQuery = -1;
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
		else if (std::strcmp( argv[argi], "-Q") == 0)
		{
			if (++argi == argc) throw std::runtime_error("argument expected for option -Q (qryno)");
			selectQuery = atoi( argv[argi]);
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

		testWeightingTitle( nofDocuments, nofTerms, nofNodes, commitSize, nofQueryies, selectDocid, selectQuery);
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


