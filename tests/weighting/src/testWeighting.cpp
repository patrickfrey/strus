/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "storageDef.hpp"
#include "strus/base/uintCompaction.hpp"
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
#include "private/skipScanArray.hpp"
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

#define STRUS_LOWLEVEL_DEBUG
#define STRUS_LOWLEVEL_DEBUG_FIELD 121,127

static strus::ErrorBufferInterface* g_errorhnd = 0;
static strus::FileLocatorInterface* g_fileLocator = 0;
static int g_verbosity = 0;
static bool g_dumpCollection = false;
static strus::PseudoRandom g_random;

typedef strus::test::Storage Storage;

struct DocTreeNode
{
	std::vector<int> titlear;
	std::vector<int> contentar;
	std::list<DocTreeNode> chld;
	strus::IndexRange field;

	DocTreeNode()
		:titlear(),contentar(),chld(),field(){}
	DocTreeNode( const std::vector<int>& titlear_, const std::vector<int>& contentar_, strus::Index startpos_)
		:titlear(titlear_),contentar(contentar_),chld(),field( startpos_, startpos_ + titlear_.size() + contentar_.size()){}
	DocTreeNode( const DocTreeNode& o)
		:titlear(o.titlear),contentar(o.contentar),chld(o.chld),field(o.field){}

	void add( const DocTreeNode& nd)
		{chld.push_back(nd); field.setEnd( chld.back().field.end());}

	void printToString( std::string& result, int indent) const
	{
		result.append( std::string( 2*indent, ' '));
		if (field.defined())
		{
			result.append( strus::string_format( "[%d,%d]", (int)field.start(), (int)field.end()));
		}
		std::vector<int>::const_iterator ti = titlear.begin(), te = titlear.end();
		for (; ti != te; ++ti)
		{
			result.append( strus::string_format(" %d", *ti));
		}
		result.append( " :");
		std::vector<int>::const_iterator xi = contentar.begin(), xe = contentar.end();
		for (; xi != xe; ++xi)
		{
			result.append( strus::string_format(" %d", *xi));
		}
		result.append("\n");
		std::list<DocTreeNode>::const_iterator ci = chld.begin(), ce = chld.end();
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

	static void collectFeatureOccurrencies( std::map<int,int>& res, const std::vector<int>& featar)
	{
		std::vector<int>::const_iterator fi = featar.begin(), fe = featar.end();
		for (; fi != fe; ++fi)
		{
			res[ *fi] += 1;
		}
	}

	void collectFeatureOccurrencies( std::map<int,int>& res) const
	{
		collectFeatureOccurrencies( res, titlear);
		collectFeatureOccurrencies( res, contentar);
		std::list<DocTreeNode>::const_iterator ci = chld.begin(), ce = chld.end();
		for (; ci != ce; ++ci)
		{
			ci->collectFeatureOccurrencies( res);
		}
	}

	static void collectFeaturePositions( std::vector<strus::Index>& res, const std::vector<int>& featar, strus::Index startpos, int featidx)
	{
		std::vector<int>::const_iterator fi = featar.begin(), fe = featar.end();
		for (strus::Index fidx=0; fi != fe; ++fi,++fidx)
		{
			if (*fi == featidx) res.push_back( startpos + fidx);
		}
	}

	void collectFeaturePositions( std::vector<strus::Index>& res, int featidx) const
	{
		collectFeaturePositions( res, titlear, field.start(), featidx);
		collectFeaturePositions( res, contentar, field.start() + titlear.size(), featidx);
		std::list<DocTreeNode>::const_iterator ci = chld.begin(), ce = chld.end();
		for (; ci != ce; ++ci)
		{
			ci->collectFeaturePositions( res, featidx);
		}
	}
};

static DocTreeNode createDocTree( int nofChilds, int nofTerms, int nofFeatures, int depth, strus::Index startpos)
{
	std::vector<int> titlear;
	std::vector<int> contentar;
	int fi = 0, fe = g_random.get( 1, g_random.get( 1, nofFeatures));
	for (; fi != fe; ++fi)
	{
		titlear.push_back( g_random.get( 1, nofTerms));
	}
	fi = 0, fe = g_random.get( 1, nofFeatures);
	for (; fi != fe; ++fi)
	{
		contentar.push_back( g_random.get( 0, nofTerms));
	}
	if (!contentar.empty() && contentar.back() != 0)
	{
		contentar.push_back( 0/*EOS*/);
	}
	if (startpos <= 0) startpos = 1;
	DocTreeNode rt( titlear, contentar, startpos);
	startpos += rt.titlear.size() + rt.contentar.size();
	if (startpos <= rt.field.start() || startpos >= strus::Constants::storage_max_position_info())
	{
		throw std::runtime_error( "test document tree too complex");
	}
	if (depth > 0)
	{
		int mi = 0, me = g_random.get( 0, nofChilds);
		for (; mi != me; ++mi)
		{
			rt.add( createDocTree( nofChilds, nofTerms, nofFeatures, depth-1, startpos));
			startpos = rt.field.end();
		}
	}
	return rt;
}

struct Query
{
	std::vector<int> features;

	Query()
		:features(){}
	Query( const std::vector<int>& features_)
		:features(features_){}
	Query( const Query& o)
		:features(o.features){}

	static std::vector<int> firstSentence( const std::vector<int>& featar)
	{
		std::vector<int>::const_iterator fi = featar.begin(), fe = featar.end();
		for (; fi != fe && *fi == 0/*EOS*/; ++fi){}
		std::vector<int>::const_iterator start = fi;
		for (; fi != fe && *fi != 0/*EOS*/; ++fi){}
		std::vector<int>::const_iterator end = fi;
		return std::vector<int>( start, end);
	}

	static Query createRandomQuery( const DocTreeNode& tree)
	{
		int nofTries = 0;
		while (++nofTries < 100)
		{
			int select = g_random.get( 0, 2+tree.chld.size());
			if (select == 0)
			{
				return Query( std::vector<int>( tree.titlear));
			}
			else if (select == 1)
			{
				return Query( firstSentence( tree.contentar));
			}
			else
			{
				std::list<DocTreeNode>::const_iterator ci = tree.chld.begin(), ce = tree.chld.end();
				int si = 0, se = select-2;
				for (; si != se && ci != ce; ++si,++ci){}
				return createRandomQuery( *ci);
			}
		}
		return Query();
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

	bool empty() const
	{
		return features.empty();
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

static void fillStructureList( std::vector<strus::test::StructureDef>& structurelist, const DocTreeNode& node)
{
	strus::IndexRange header( node.field.start(), node.field.start() + node.titlear.size());
	strus::IndexRange content( header.end(), header.end() + node.contentar.size());
	strus::IndexRange allcontent( header.end(), node.field.end());
	if (header.defined())
	{
		if (content.defined())
		{
			structurelist.push_back( strus::test::StructureDef( "title", header, content));
		}
		if (content != allcontent)
		{
			structurelist.push_back( strus::test::StructureDef( "title", header, allcontent));
		}
	}
	std::list<DocTreeNode>::const_iterator ci = node.chld.begin(), ce = node.chld.end();
	for (; ci != ce; ++ci)
	{
		fillStructureList( structurelist, *ci);
	}
}

struct FeaturePos
{
	int featidx;
	strus::Index pos;

	FeaturePos( int featidx_, strus::Index pos_)
		:featidx(featidx_),pos(pos_){}
	FeaturePos( const FeaturePos& o)
		:featidx(o.featidx),pos(o.pos){}

	struct FindPosCompare
	{
		FindPosCompare(){}
		bool operator()( const FeaturePos& aa, strus::Index searchpos) const
		{
			return aa.pos < searchpos;
		}
	};
};

static void fillFeaturePosList( std::vector<FeaturePos>& res, strus::Index startpos, const std::vector<int>& featar)
{
	std::vector<int>::const_iterator fi = featar.begin(), fe = featar.end();
	for (int fidx=0; fi != fe; ++fi,++fidx)
	{
		res.push_back( FeaturePos( *fi, startpos + fidx));
	}
}

static void fillFeaturePosList( std::vector<FeaturePos>& res, const DocTreeNode& node)
{
	fillFeaturePosList( res, node.field.start(), node.titlear);
	fillFeaturePosList( res, node.field.start() + node.titlear.size()/*startpos*/, node.contentar);

	std::list<DocTreeNode>::const_iterator ci = node.chld.begin(), ce = node.chld.end();
	for (; ci != ce; ++ci)
	{
		fillFeaturePosList( res, *ci);
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

static void fillTitles( std::vector<LabeledTitle>& titlelist, const DocTreeNode& node, const strus::IndexRange& field, int depth)
{
	if (node.field.cover( field))
	{
		std::vector<int>::const_iterator fi = node.titlear.begin(), fe = node.titlear.end();
		std::string titletext;
		for (; fi != fe; ++fi)
		{
			if (!titletext.empty()) titletext.push_back(' ');
			titletext.append( strus::string_format( "%d", *fi));
		}
		if (!titletext.empty()) titlelist.push_back( LabeledTitle( titletext, depth));
		std::list<DocTreeNode>::const_iterator ci = node.chld.begin(), ce = node.chld.end();
		for (; ci != ce; ++ci)
		{
			fillTitles( titlelist, *ci, field, depth+1);
		}
	}
}

static std::string getEntityString( int featidx)
{
	static char const* cnt[] = {"","one","two","three","four","five","six","seven","eight","nine","ten"};
	static char const* ar[]  = {"","ten","twenty","thirty","fourty","fifty","sixty","seventy","eighty","ninety","hundred"};
	if (featidx <= 1000)
	{
		if (featidx <= 100)
		{
			if (featidx % 10 == 0)
			{
				return ar[ featidx / 10];
			}
		}
		else if (featidx % 10 == 0)
		{
			return std::string( cnt[ featidx / 100]) + "hundred" + getEntityString( featidx % 100);
		}
	}
	else if (featidx <= 10000)
	{
		return std::string( cnt[ featidx / 1000]) + "thousand" + getEntityString( featidx % 1000);
	}
	return std::string();
}

static void fillFeatureList( std::vector<strus::test::Feature>& featurelist, strus::Index startpos, const std::vector<int>& featar)
{
	std::vector<int>::const_iterator fi = featar.begin(), fe = featar.end();
	for (int fidx=0; fi != fe; ++fi,++fidx)
	{
		strus::Index pos = startpos + fidx;
		if (*fi == 0/*EOS*/)
		{
			featurelist.push_back( strus::test::Feature( strus::test::Feature::SearchIndex, "eos", "", pos));
			featurelist.push_back( strus::test::Feature( strus::test::Feature::ForwardIndex, "orig", ".", pos));
		}
		else
		{
			std::string word = strus::string_format( "f%d", *fi);
			featurelist.push_back( strus::test::Feature( strus::test::Feature::SearchIndex, "word", word, pos));
			std::string orig = strus::string_format( "%d", *fi);
			featurelist.push_back( strus::test::Feature( strus::test::Feature::ForwardIndex, "orig", orig, pos));
			std::string entitystr = getEntityString( *fi);
			if (!entitystr.empty())
			{
				featurelist.push_back( strus::test::Feature( strus::test::Feature::ForwardIndex, "entity", entitystr, pos));
			}
			featurelist.push_back( strus::test::Feature( strus::test::Feature::ForwardIndex, "tag", (*fi & 1) == 0 ? "even":"odd", pos));
		}
	}
}

static void fillFeatureList( std::vector<strus::test::Feature>& featurelist, const DocTreeNode& node)
{
	fillFeatureList( featurelist, node.field.start(), node.titlear);
	fillFeatureList( featurelist, node.field.start() + node.titlear.size()/*startpos*/, node.contentar);

	std::list<DocTreeNode>::const_iterator ci = node.chld.begin(), ce = node.chld.end();
	for (; ci != ce; ++ci)
	{
		fillFeatureList( featurelist, *ci);
	}
}


struct WeightingConfig
{
	int distance_imm;
	int distance_close;
	int distance_near;
	int distance_collect;
	float minClusterSize;
	int nofSummarySentences;
	int maxNofSummarySentenceWords;
	double minFfWeight;
	int avgDocLength;
	double b;
	double k1;
	double maxdf;
	int maxNofRanks;
};
static WeightingConfig g_weightingConfig = {
	2/*distance_imm*/,
	8/*distance_close*/,
	40/*distance_near*/,
	6/*distance_collect*/,
	0.0/*minClusterSize*/,
	1/*nofSummarySentences*/,
	100/*maxNofSummarySentenceWords*/,
	0.1/*minFfWeight*/,
	10/*avgDocLength*/,
	0.75/*b*/,
	1.5/*k1*/,
	1.0/*maxdf*/,
	500/*maxNofRanks*/
};

struct WeightedPos
{
	int featidx;			//< feature index matched
	int qryidx;			//< query feature index counted fo the match
	strus::Index pos;		//< document ordinal position of the match
	double ff;			//< feature frequency value assigned

	WeightedPos( int featidx_, int qryidx_, strus::Index pos_, double ff_)
		:featidx(featidx_),qryidx(qryidx_),pos(pos_),ff(ff_){}
	WeightedPos( const WeightedPos& o)
		:featidx(o.featidx),qryidx(o.qryidx),pos(o.pos),ff(o.ff){}
};

struct WeightedEntity
{
	std::string name;
	double weight;

	WeightedEntity()
		:name(),weight(0.0){}
	WeightedEntity( const std::string& name_, double weight_)
		:name(name_),weight(weight_){}
	WeightedEntity( const WeightedEntity& o)
		:name(o.name),weight(o.weight){}

	bool operator < (const WeightedEntity& o) const
	{
		if (strus::Math::isequal( weight, o.weight))
		{
			return name < o.name;
		}
		else
		{
			return weight < o.weight;
		}
	}

	bool operator > (const WeightedEntity& o) const
	{
		if (strus::Math::isequal( weight, o.weight))
		{
			return name > o.name;
		}
		else
		{
			return weight > o.weight;
		}
	}
};

struct Statistics
{
	Statistics( const std::map<int,int>& dfmap_, int nofDocuments_)
		:dfmap(dfmap_),nofDocuments(nofDocuments_){}
	Statistics( const Statistics& o)
		:dfmap(o.dfmap),nofDocuments(o.nofDocuments){}
	Statistics()
		:dfmap(),nofDocuments(){}

	std::map<int,int> dfmap;
	int nofDocuments;

	bool defined() const
	{
		return nofDocuments != 0;
	}

	double idf( int featidx) const
	{
		std::map<int,int>::const_iterator ni = dfmap.find( featidx);
		int df = (ni == dfmap.end()) ? 0 : ni->second;
		double rt = strus::Math::log10( (nofDocuments - df + 0.5) / (df + 0.5));
		if (rt < 0.00001)
		{
			//... avoid negative IDFs and to small idfs
			rt = 0.00001;
		}
		return rt;
	}
	bool isStopword( int featidx) const
	{
		std::map<int,int>::const_iterator ni = dfmap.find( featidx);
		if (ni == dfmap.end()) return false;
		double df = (ni == dfmap.end()) ? 0 : ni->second;
		return (df > g_weightingConfig.maxdf * (double)nofDocuments);
	}
};

struct FeatureVisit
{
	int idx;
	std::vector<int> featidxar;

	FeatureVisit() :idx(0),featidxar() {}
	FeatureVisit( const FeatureVisit& o) :idx(o.idx),featidxar(o.featidxar){}
};

struct FeatureVisitMap
{
	std::map<int,FeatureVisit> map;

	explicit FeatureVisitMap( const std::vector<int>& queryFeatures)
	{
		std::vector<int>::const_iterator
			qi = queryFeatures.begin(), qe = queryFeatures.end();
		for (int qidx=0; qi != qe; ++qi,++qidx)
		{
			map[ *qi].featidxar.push_back( qidx);
		}
	}

	int nextQueryFeatIndex( int featidx)
	{
		std::map<int,FeatureVisit>::iterator mi = map.find( featidx);
		if (mi == map.end())
		{
			return -1;
		}
		else
		{
			int idx = mi->second.idx++;
			mi->second.idx = mi->second.idx % (int)mi->second.featidxar.size();
			return mi->second.featidxar[ idx];
		}
	}
};

struct Document
{
	std::string docid;
	std::vector<DocTreeNode> doctreelist;
	std::vector<strus::test::StructureDef> structurelist;
	std::vector<strus::test::Feature> featurelist;
	std::vector<FeaturePos> featposlist;

	Document()
		:docid(),doctreelist(),structurelist(),featurelist(),featposlist(){}
	Document( int didx, const std::vector<DocTreeNode>& doctreelist_)
		:docid(strus::string_format("D%d", didx)),doctreelist(doctreelist_)
		,structurelist(),featurelist(),featposlist()
	{
		std::vector<DocTreeNode>::const_iterator ti = doctreelist.begin(), te = doctreelist.end();
		for (; ti != te; ++ti)
		{
			fillStructureList( structurelist, *ti);
			fillFeatureList( featurelist, *ti);
			fillFeaturePosList( featposlist, *ti);
		}
	}
	Document( const Document& o)
		:docid(o.docid),doctreelist(o.doctreelist)
		,structurelist(o.structurelist),featurelist(o.featurelist),featposlist(o.featposlist){}

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

		std::vector<DocTreeNode> doctreelist_;
		int startpos = 0;
		do
		{
			DocTreeNode doctree_ = createDocTree( nofChilds, nofTerms, nofFeatures, depth, startpos);
			startpos = doctree_.field.end();
			doctreelist_.push_back( doctree_);
		} while (g_random.get( 0, 3) == 1);
		return Document( didx+1, doctreelist_);
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
		doc->setMetaData( "doclen", strus::NumericVariant::asint( doctreelist.empty() ? 0 : doctreelist.back().field.end() - 1));

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
		std::vector<DocTreeNode>::const_iterator
			ti = doctreelist.begin(), te = doctreelist.end();
		for (; ti != te; ++ti)
		{
			ti->printToString( rt, 0);
		}
		return rt;
	}

	std::set<int> getTitleFeatures( const strus::IndexRange& titlefield, const std::vector<int>& features) const
	{
		strus::SkipScanArray<FeaturePos,strus::Index,FeaturePos::FindPosCompare>
			ar( featposlist.data(), featposlist.size());
		int startidx = ar.upperbound( titlefield.start());
		int endidx = ar.upperbound( titlefield.end());
		if (startidx < 0) return std::set<int>();
		if (endidx < 0) endidx = featposlist.size();

		std::set<int> rt;
		std::vector<FeaturePos>::const_iterator
			fi = featposlist.begin() + startidx,
			fe = featposlist.begin() + endidx;
		for (; fi != fe; ++fi)
		{
			if (std::find( features.begin(), features.end(), fi->featidx) != features.end())
			{
				rt.insert( fi->featidx);
			}
		}
		return rt;
	}

	void collectDfStats( std::map<int,int>& res) const
	{
		std::map<int,int> tfmap;
		std::vector<DocTreeNode>::const_iterator di = doctreelist.begin(), de = doctreelist.end();
		for (; di != de; ++di)
		{
			di->collectFeatureOccurrencies( tfmap);
		}
		std::map<int,int>::const_iterator ti = tfmap.begin(), te = tfmap.end();
		for (; ti != te; ++ti)
		{
			res[ ti->first] += ti->second ? 1 : 0;
		}
	}

	struct TouchCounter
	{
		int weightedNeighbours;
		int minClusterSize;
		double minFfWeight;
		int dist_imm_cnt;
		int dist_close_cnt;
		int dist_sent_cnt;
		int dist_near_cnt;
		int dist_title_cnt;

		TouchCounter( int weightedNeighbours_, int minClusterSize_, double minFfWeight_)
		{
			weightedNeighbours = weightedNeighbours_;
			minClusterSize = minClusterSize_;
			minFfWeight = minFfWeight_;
			dist_imm_cnt = 0;
			dist_close_cnt = 0;
			dist_sent_cnt = 0;
			dist_near_cnt = 0;
			dist_title_cnt = 0;
		}

		int count() const
		{
			return dist_imm_cnt + dist_close_cnt + dist_sent_cnt + dist_near_cnt + dist_title_cnt;
		}
		double touch_weight( int touches) const
		{
			return (float)(touches * touches)/(float)(weightedNeighbours * weightedNeighbours);
		}
		double weight() const
		{
			double ww =
				((dist_imm_cnt) ? 1.0 : 0.0)
				+ touch_weight( dist_imm_cnt + dist_close_cnt)
				+ touch_weight( dist_imm_cnt + dist_close_cnt + dist_sent_cnt)
				+ touch_weight( dist_imm_cnt + dist_close_cnt + dist_sent_cnt + dist_near_cnt + dist_title_cnt);
			if (count() >= minClusterSize)
			{
				return (1.0 - minFfWeight) * (ww / 4) + minFfWeight;
			}
			else
			{
				return 0.0;
			}
		}
	};

	bool findTitleFeature( const DocTreeNode& node, strus::Index pos, int featidx) const
	{
		if (node.field.contain( pos) && pos >= node.field.start() + (strus::Index)node.titlear.size())
		{
			if (node.titlear.end() != std::find( node.titlear.begin(), node.titlear.end(), featidx))
			{
				return true;
			}
			std::list<DocTreeNode>::const_iterator ci = node.chld.begin(), ce = node.chld.end();
			for (; ci != ce; ++ci)
			{
				if (findTitleFeature( *ci, pos, featidx)) return true;
			}
		}
		return false;
	}

	bool hasTitleFeature( strus::Index pos, int featidx) const
	{
		std::vector<DocTreeNode>::const_iterator
			ti = doctreelist.begin(), te = doctreelist.end();
		for (; ti != te; ++ti)
		{
			if (findTitleFeature( *ti, pos, featidx))
			{
				return true;
			}
		}
		return false;
	}

	std::vector<FeaturePos>::const_iterator findNeighbourFeature(
			std::vector<FeaturePos>::const_iterator fi,
			int searchFeatidx,
			int dist,
			const std::set<int>& usedfeats,
			bool stopOnEos) const
	{
		std::vector<FeaturePos>::const_iterator prev_fi = fi;
		if (prev_fi != featposlist.begin())
		{
			for (--prev_fi; prev_fi >= featposlist.begin()
				&& fi->pos - prev_fi->pos - 1/*feature length*/ < dist;
				--prev_fi)
			{
				if (usedfeats.end() != usedfeats.find( prev_fi - featposlist.begin()))
				{
					// ... already counted as touch
					continue;
				}
				if (prev_fi->featidx == 0/*EOS*/)
				{
					if (stopOnEos) break;
				}
				else if (searchFeatidx == prev_fi->featidx)
				{
					return prev_fi;
				}
			}
		}
		std::vector<FeaturePos>::const_iterator next_fi = fi;
		for (++next_fi; next_fi != featposlist.end()
			&& next_fi->pos - fi->pos - 1/*feature length*/ < dist;
			++next_fi)
		{
			if (usedfeats.end() != usedfeats.find( next_fi - featposlist.begin()))
			{
				// ... already counted as touch
				continue;
			}
			if (next_fi->featidx == 0/*EOS*/)
			{
				if (stopOnEos) break;
			}
			else if (searchFeatidx == next_fi->featidx)
			{
				return next_fi;
			}
		}
		return featposlist.end();
	}

	void collectWeightedPos(
			std::vector<WeightedPos>& res, 
			const std::vector<int>& queryFeatures,
			const strus::IndexRange& contentfield,
			bool includingWeightsForTitle) const
	{
		strus::SkipScanArray<FeaturePos,strus::Index,FeaturePos::FindPosCompare>
			ar( featposlist.data(), featposlist.size());
		int startidx = contentfield.defined() ? ar.upperbound( contentfield.start()) : 0;
		int endidx = contentfield.defined() ? ar.upperbound( contentfield.end()) : featposlist.size();
		if (startidx < 0) return;
		if (endidx < 0) endidx = featposlist.size();

		int minClusterSize = g_weightingConfig.minClusterSize * queryFeatures.size() + 0.5;
		if (minClusterSize == 0) minClusterSize = 1;

		if (queryFeatures.size() == 1)
		{
			std::vector<FeaturePos>::const_iterator
				fi = featposlist.begin() + startidx,
				fe = featposlist.begin() + endidx;
			for (; fi != fe; ++fi)
			{
				if (fi->featidx == queryFeatures[0])
				{
					res.push_back( WeightedPos( fi->featidx, 0/*qryidx*/, fi->pos, 1.0));
				}
			}
		}
		else
		{
			FeatureVisitMap featureVisitMap( queryFeatures);
			std::vector<FeaturePos>::const_iterator
				fi = featposlist.begin() + startidx,
				fe = featposlist.begin() + endidx;
			for (; fi != fe; ++fi)
			{
				int qryidx = featureVisitMap.nextQueryFeatIndex( fi->featidx);
				if (qryidx >= 0)
				{
					std::vector<int> othfeats = queryFeatures;
					othfeats.erase( othfeats.begin() + qryidx);
					std::vector<int> nomatchfeats;
					std::set<int> usedfeats;
	
					int weightedNeighbours = queryFeatures.size()-1;
					TouchCounter touchCounter( weightedNeighbours, minClusterSize, g_weightingConfig.minFfWeight);
	
					std::vector<int>::const_iterator
						oi = othfeats.begin(), oe = othfeats.end();
					for (; oi != oe; ++oi)
					{
						std::vector<FeaturePos>::const_iterator neighbour_fi;
						if (featposlist.end() != (neighbour_fi = findNeighbourFeature( fi, *oi, g_weightingConfig.distance_imm, usedfeats, true/*stop on EOS*/)))
						{
							touchCounter.dist_imm_cnt++;
							usedfeats.insert( neighbour_fi - featposlist.begin());
						}
						else if (featposlist.end() != (neighbour_fi = findNeighbourFeature( fi, *oi, g_weightingConfig.distance_close, usedfeats, true/*stop on EOS*/)))
						{
							touchCounter.dist_close_cnt++;
							usedfeats.insert( neighbour_fi - featposlist.begin());
						}
						else if (featposlist.end() != (neighbour_fi = findNeighbourFeature( fi, *oi, g_weightingConfig.maxNofSummarySentenceWords, usedfeats, true/*stop on EOS*/)))
						{
							touchCounter.dist_sent_cnt++;
							usedfeats.insert( neighbour_fi - featposlist.begin());
						}
						else if (featposlist.end() != (neighbour_fi = findNeighbourFeature( fi, *oi, g_weightingConfig.distance_near, usedfeats, false/*stop on EOS*/)))
						{
							touchCounter.dist_near_cnt++;
							usedfeats.insert( neighbour_fi - featposlist.begin());
						}
						else
						{
							nomatchfeats.push_back( *oi);
						}
					}
					if (touchCounter.count() >= minClusterSize)
					{
						if (includingWeightsForTitle)
						{
							std::vector<int>::const_iterator
								xi = nomatchfeats.begin(), xe = nomatchfeats.end();
							for (; xi != xe; ++xi)
							{
								if (hasTitleFeature( fi->pos, *xi))
								{
									touchCounter.dist_title_cnt++;
								}
							}
						}
#ifdef STRUS_LOWLEVEL_DEBUG
						if (contentfield == strus::IndexRange(STRUS_LOWLEVEL_DEBUG_FIELD))
						{
							std::cerr << strus::string_format( "DEBUG expected weight pos=%d, featidx=%d, qryidx=%d, ff=%.8f, touch: {imm=%d, close=%d, sent=%d, near=%d, title=%d}", (int)fi->pos, (int)fi->featidx, (int)qryidx, touchCounter.weight(), touchCounter.dist_imm_cnt, touchCounter.dist_close_cnt, touchCounter.dist_sent_cnt, touchCounter.dist_near_cnt, touchCounter.dist_title_cnt) << std::endl;
						}
#endif
						res.push_back( WeightedPos( fi->featidx, qryidx, fi->pos, touchCounter.weight()));
					}
				}
			}
		}
	}

	std::vector<WeightedPos> getWeightedPos( const std::vector<int>& queryFeatures, const strus::IndexRange& contentfield) const
	{
		std::vector<WeightedPos> rt;
		collectWeightedPos( rt, queryFeatures, contentfield, false/*!includingWeightsForTitle*/);
		return rt;
	}

	std::vector<WeightedPos> getWeightedPosIncludingTitleWeights( const std::vector<int>& queryFeatures, const strus::IndexRange& contentfield) const
	{
		std::vector<WeightedPos> rt;
		collectWeightedPos( rt, queryFeatures, contentfield, true/*includingWeightsForTitle*/);
		return rt;
	}

	static double postingsWeight_bm25( const Statistics& statistics, int doclen, int featidx, double ff)
	{
		double b = g_weightingConfig.b;
		double k1 = g_weightingConfig.k1;
		double featureWeight = statistics.idf( featidx);
		double rt;
		if (b)
		{
			double rel_doclen = (double)doclen / g_weightingConfig.avgDocLength;
			rt = featureWeight
					* (ff * (k1 + 1.0))
					/ (ff + k1
						* (1.0 - b + b * rel_doclen));
		}
		else
		{
			rt = featureWeight
					* (ff * (k1 + 1.0))
					/ (ff + k1 * 1.0);
		}
		return rt;
	}

	std::map<int,double> getQueryFeatFfMap( const std::vector<WeightedPos>& wpos, const std::vector<int>& queryFeatures) const
	{
		// Collect the proximity weighted feature frequencies:
		std::vector<WeightedPos>::const_iterator wi = wpos.begin(), we = wpos.end();
		std::map<int,double> rt;
		for (; wi != we; ++wi)
		{
			if (queryFeatures.end() != std::find( queryFeatures.begin(), queryFeatures.end(), wi->featidx))
			{
				rt[ wi->qryidx] += wi->ff;
			}
		}
		return rt;
	}

	int getSingleFeatFf( int featidx, const strus::IndexRange& field) const
	{
		int rt = 0;
		std::vector<FeaturePos>::const_iterator fi = featposlist.begin(), fe = featposlist.end();
		for (; fi != fe; ++fi)
		{
			if (fi->featidx == featidx && (!field.defined() || field.contain( fi->pos))) ++rt; 
		}
		return rt;
	}

	double weightField_bm25( const std::vector<int>& queryFeatures, const std::map<int,double>& queryFeatFfMap, const Statistics& statistics, const strus::IndexRange& docfield) const
	{
		double rt = 0.0;
		std::vector<int>::const_iterator qi = queryFeatures.begin(), qe = queryFeatures.end();
		for (int qidx=0; qi != qe; ++qi,++qidx)
		{
			int doclen = docfield.end() - docfield.start();
			std::map<int,double>::const_iterator fi = queryFeatFfMap.find( qidx);
			double ff = fi == queryFeatFfMap.end() ? 0.0 : fi->second;
			rt += postingsWeight_bm25( statistics, doclen, *qi, ff);
		}
		return rt;
	}

	std::string getContent( const strus::IndexRange& field) const
	{
		std::string rt;
		std::vector<FeaturePos>::const_iterator
			fi = featposlist.begin(), fe = featposlist.end();
		for (; fi != fe && fi->pos < field.start(); ++fi){}
		for (; fi != fe && fi->pos < field.end(); ++fi)
		{
			std::string orig = fi->featidx == 0/*EOS*/
					? "."
					: strus::string_format( "%d", fi->featidx);
			std::string entitystr = getEntityString( fi->featidx);
			if (!entitystr.empty())
			{
				if (!rt.empty() && rt[ rt.size()-1] != ' ') rt.push_back(' ');
				rt.append( "[");
				rt.append( entitystr);
				rt.append( "] ");
				rt.append( orig);
			}
			else
			{
				if (!rt.empty() && rt[ rt.size()-1] != ' ') rt.push_back(' ');
				rt.append( orig);
			}
		}
		return rt;
	}

	std::vector<WeightedEntity>
		collectWeightedNearMatches( 
			std::vector<WeightedPos> wpos, int dist,
			const Statistics& statistics, int maxNofRanks) const
	{
		std::map<std::string,double> map;
		std::vector<WeightedPos>::const_iterator wi = wpos.begin(), we = wpos.end();
		for (; wi != we; ++wi)
		{
			strus::Index startPos = wi->pos > dist ? wi->pos - dist : 1;
			strus::Index endPos = wi->pos + dist + 1;
			double ww = wi->ff * statistics.idf( wi->featidx);
			std::vector<FeaturePos>::const_iterator
				fi = featposlist.begin(), fe = featposlist.end();
			for (; fi != fe && fi->pos < startPos; ++fi){}
			for (; fi != fe && fi->pos < endPos; ++fi)
			{
				std::string orig = strus::string_format( "%d", fi->featidx);
				std::string entitystr = getEntityString( fi->featidx);
				std::string tagstr = (fi->featidx & 1) == 0 ? "even":"odd";
				std::string featstr = tagstr + "#" + ((!entitystr.empty()) ? entitystr : orig);
				map[ featstr] += ww;
			}
		}
		std::vector<WeightedEntity> rt;
		std::map<std::string,double>::const_iterator mi = map.begin(), me = map.end();
		for (; mi != me; ++mi)
		{
			rt.push_back( WeightedEntity( mi->first, mi->second));
		}
		int nofRanked = rt.size();
		if (maxNofRanks <= 0 || maxNofRanks > nofRanked)
		{
			std::sort( rt.begin(), rt.end(), std::greater<WeightedEntity>());
		}
		else
		{
			std::nth_element( rt.begin(), rt.begin()+maxNofRanks, rt.end(), std::greater<WeightedEntity>());
			rt.resize( maxNofRanks);
			std::sort( rt.begin(), rt.end(), std::greater<WeightedEntity>());
		}
		return rt;
	}

	void collectRanklists_bm25pff( std::vector<strus::WeightedField>& res, const Statistics& statistics, const std::vector<int>& queryFeatures, const DocTreeNode& node) const
	{
		strus::Index contentstart = node.field.start() + node.titlear.size();
		strus::IndexRange contentfield( contentstart, contentstart + node.contentar.size());
		strus::IndexRange allcontentfield( contentstart, node.field.end());

		std::vector<WeightedPos> wpos = getWeightedPosIncludingTitleWeights( queryFeatures, contentfield);
		std::map<int,double> queryFeatFfMap = getQueryFeatFfMap( wpos, queryFeatures);
		double ww = weightField_bm25( queryFeatures, queryFeatFfMap, statistics, contentfield);
		if (ww > 0.0)
		{
			res.push_back( strus::WeightedField( contentfield, ww));
		}
		if (contentfield != allcontentfield)
		{
			wpos = getWeightedPosIncludingTitleWeights( queryFeatures, allcontentfield);
			queryFeatFfMap = getQueryFeatFfMap( wpos, queryFeatures);
			ww = weightField_bm25( queryFeatures, queryFeatFfMap, statistics, allcontentfield);
			if (ww > 0.0)
			{
				res.push_back( strus::WeightedField( allcontentfield, ww));
			}
		}
		std::list<DocTreeNode>::const_iterator ci = node.chld.begin(), ce = node.chld.end();
		for (; ci != ce; ++ci)
		{
			collectRanklists_bm25pff( res, statistics, queryFeatures, *ci);
		}
	}

	void collectRanklists_bm25pff( std::vector<strus::WeightedField>& res, const Statistics& statistics, const std::vector<int>& queryFeatures) const
	{
		if (doctreelist.empty()) return;
		if (queryFeatures.empty()) return;
		if (queryFeatures.size() == 1)
		{
			// ... Fallback to bm25
			int featidx = queryFeatures[0];
			int ff = getSingleFeatFf( featidx, strus::IndexRange());
			int doclen = doctreelist.back().field.end()-1;
			double ww = postingsWeight_bm25( statistics, doclen, featidx, ff);
			if (ww > 0.0)
			{
				res.push_back( strus::WeightedField( strus::IndexRange(), ww));
			}
		}
		else
		{
			strus::IndexRange contentfield( 1, doctreelist.back().field.end());

			std::vector<WeightedPos> wpos = getWeightedPosIncludingTitleWeights( queryFeatures, contentfield);
			std::map<int,double> queryFeatFfMap = getQueryFeatFfMap( wpos, queryFeatures);
			double ww = weightField_bm25( queryFeatures, queryFeatFfMap, statistics, contentfield);
			if (ww > 0.0)
			{
				res.push_back( strus::WeightedField( strus::IndexRange(), ww));
			}
			std::vector<DocTreeNode>::const_iterator di = doctreelist.begin(), de = doctreelist.end();
			for (; di != de; ++di)
			{
				collectRanklists_bm25pff( res, statistics, queryFeatures, *di);
			}
		}
	}

	std::vector<strus::WeightedField> calculateRanklist_bm25pff( const Statistics& statistics, const std::vector<int>& queryFeatures) const
	{
		std::vector<strus::WeightedField> rt;
		collectRanklists_bm25pff( rt, statistics, queryFeatures);
		return rt;
	}

	double calculate_bm25( const Statistics& statistics, const std::vector<int>& queryFeatures, bool compacted_ff) const
	{
		double rt = 0.0;
		std::vector<int>::const_iterator
			qi = queryFeatures.begin(), qe = queryFeatures.end();
		for (; qi != qe; ++qi)
		{
			int featidx = *qi;
			int ff = getSingleFeatFf( featidx, strus::IndexRange());
			if (compacted_ff)
			{
				ff = strus::uintFromCompaction( strus::compactUint( ff));
			}
			int doclen = doctreelist.back().field.end()-1;
			rt += postingsWeight_bm25( statistics, doclen, featidx, ff);
		}
		return rt;
	}

	std::vector<strus::Index> getFeaturePositions( int featidx) const
	{
		std::vector<strus::Index> rt;
		std::vector<DocTreeNode>::const_iterator di = doctreelist.begin(), de = doctreelist.end();
		for (; di != de; ++di)
		{
			di->collectFeaturePositions( rt, featidx);
		}
		return rt;
	}

	strus::WeightedField getFirstMatchSentence( int featidx) const
	{
		if (featposlist.empty()) return strus::WeightedField();
		strus::Index start = 1;
		strus::Index end = featposlist.back().pos + 1;
		std::vector<FeaturePos>::const_iterator
			fi = featposlist.begin(), fe = featposlist.end();
		for (; fi != fe && fi->featidx != featidx; ++fi)
		{
			if (fi->featidx == 0/*EOS*/) start = fi->pos + 1;
		}
		for (; fi != fe && fi->featidx != 0/*EOS*/; ++fi){}
		if (fi != fe) end = fi->pos;
		return strus::WeightedField( strus::IndexRange( start, end), 1.0);
	}

	strus::IndexRange getCoveringSentence( strus::Index pos) const
	{
		if (featposlist.empty()) return strus::IndexRange();
		strus::Index start = 1;
		strus::Index end = featposlist.back().pos + 1;
		std::vector<FeaturePos>::const_iterator
			fi = featposlist.begin(), fe = featposlist.end();
		for (; fi != fe && fi->pos != pos; ++fi)
		{
			if (fi->featidx == 0/*EOS*/) start = fi->pos + 1;
		}
		for (; fi != fe && fi->featidx != 0/*EOS*/; ++fi){}
		if (fi != fe) end = fi->pos;
		return strus::IndexRange( start, end);
	}
};

struct Collection
{
	std::vector<Document> doclist;
	Statistics statistics;

	Collection( int nofDocuments, int nofTerms, int nofNodes)
		:doclist(),statistics()
	{
		statistics.nofDocuments = nofDocuments;
		int di = 0, de = nofDocuments;
		for (; di != de; ++di)
		{
			doclist.push_back( Document::createRandom( di, nofTerms, nofNodes));
			doclist.back().collectDfStats( statistics.dfmap);
		}
		
	}
	Collection( const Collection& o)
		:doclist(o.doclist),statistics(o.statistics){}

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
					std::cerr << "\ninsert " << di->docid << ":" << std::endl;
					std::cerr << di->tostring() << std::endl;
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

	void check( strus::StorageClientInterface* storage)
	{
		std::vector<Document>::const_iterator di = doclist.begin(), de = doclist.end();
		for (; di != de; ++di)
		{
			if (g_verbosity >= 2)
			{
				std::cerr << "check " << di->docid << std::endl;
			}
			strus::local_ptr<strus::StorageDocumentInterface>
				checker( storage->createDocumentChecker( di->docid));
			if (!checker.get()) throw strus::runtime_error("error creating document to insert");

			di->build( checker.get());

			if (g_errorhnd->hasError())
			{
				throw strus::runtime_error( "document check failed: %s", g_errorhnd->fetchError());
			}
		}
	}

	void check( strus::StorageClientInterface* storage, const std::string& selectDocid)
	{
		Document data = getDocument( selectDocid);
		strus::local_ptr<strus::StorageDocumentInterface>
			checker( storage->createDocumentChecker( selectDocid));
		if (!checker.get())
		{
			throw strus::runtime_error( "document check failed: %s", g_errorhnd->fetchError());
		}
		data.build( checker.get());
		if (g_errorhnd->hasError())
		{
			throw strus::runtime_error( "document check failed: %s", g_errorhnd->fetchError());
		}
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
		const DocTreeNode& tree = doc.doctreelist[ g_random.get( 0, doc.doctreelist.size())];
		return Query::createRandomQuery( tree);
	}

	std::vector<Query> randomQueries( int nofQueries) const
	{
		std::vector<Query> rt;
		int qi = 0, qe = nofQueries;
		for (; qi != qe; ++qi)
		{
			rt.push_back( randomQuery());
			if (rt.back().empty())
			{
				rt.resize( rt.size()-1);
				--qi;
			}
		}
		return rt;
	}

	strus::WeightedField getBestSentence( const Document& doc, const std::vector<WeightedPos>& wpos, int windowSize) const
	{
		strus::WeightedField rt;
		double max_weight = 0.0;
		strus::Index bestpos = 0;
		std::vector<WeightedPos>::const_iterator wi = wpos.begin(), we = wpos.end();
		for (; wi != we; ++wi)
		{
			double weight = 0.0;
			std::vector<WeightedPos>::const_iterator pi = wi;
			for (; pi >= wpos.begin() && pi->pos + windowSize > wi->pos; --pi)
			{
				weight += pi->ff * statistics.idf( pi->featidx);
			}
			if (weight > max_weight)
			{
				bestpos = wi->pos;
				max_weight = weight;
			}
		}
		if (bestpos)
		{
			strus::IndexRange sent = doc.getCoveringSentence( bestpos);
			rt = strus::WeightedField( sent, max_weight);
		}
		return rt;
	}

	strus::QueryResult expectedResult_bm25pff( int maxNofRanks, const Query& query, const strus::StorageClientInterface* storage) const
	{
		std::vector<strus::ResultDocument> ranks;
		std::vector<Document>::const_iterator di = doclist.begin(), de = doclist.end();
		for (; di != de; ++di)
		{
			strus::Index docno = storage->documentNumber( di->docid);
			if (docno)
			{
				std::vector<strus::WeightedField>
					weightedFields = di->calculateRanklist_bm25pff( statistics, query.features);
				std::vector<strus::WeightedField>::const_iterator
					wi = weightedFields.begin(), we = weightedFields.end();
				for (; wi != we; ++wi)
				{
					strus::WeightedDocument wdoc( docno, wi->field(), wi->weight());
					std::vector<strus::SummaryElement> summary;
					summary.push_back( strus::SummaryElement( "docid", di->docid, 1.0));
					if (wi->field().defined())
					{
						std::vector<LabeledTitle> titlelist;
						std::vector<DocTreeNode>::const_iterator
							ti = di->doctreelist.begin(), te = di->doctreelist.end();
						for (; ti != te; ++ti)
						{
							fillTitles( titlelist, *ti, wi->field(), 0/*depth*/);
						}
						std::vector<LabeledTitle>::const_iterator
							li = titlelist.begin(), le = titlelist.end();
						for (int lidx=0; li != le; ++li,++lidx)
						{
							summary.push_back( strus::SummaryElement( "header", li->text, 1.0, li->hierarchy));
						}
					}
					std::vector<int> nonStopwordQueryFeatures;
					std::vector<int>::const_iterator qi = query.features.begin(), qe = query.features.end();
					for (; qi != qe; ++qi)
					{
						if (!statistics.isStopword( *qi)) nonStopwordQueryFeatures.push_back( *qi);
					}
					if (nonStopwordQueryFeatures.empty())
					{}
					else if (nonStopwordQueryFeatures.size() == 1)
					{
						strus::WeightedField firstsent = di->getFirstMatchSentence( nonStopwordQueryFeatures[0]);
						if (firstsent.field().defined())
						{
							std::string content = di->getContent( firstsent.field());
							summary.push_back( strus::SummaryElement( "phrase", content, firstsent.weight()));
						}
					}
					else
					{
						std::vector<WeightedPos> wpos = di->getWeightedPos( nonStopwordQueryFeatures, wi->field());
						int windowSize = g_weightingConfig.maxNofSummarySentenceWords * g_weightingConfig.nofSummarySentences;
						strus::WeightedField bestsent = getBestSentence( *di, wpos, windowSize);
						if (bestsent.field().defined())
						{
							std::string content = di->getContent( bestsent.field());
							summary.push_back( strus::SummaryElement( "phrase", content, bestsent.weight()));
						}
					}
					ranks.push_back( strus::ResultDocument( wdoc, summary));
				}
			}
		}
		std::vector<strus::SummaryElement> summary;
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
		return strus::QueryResult( 0/*evaluationPass*/, nofRanked, nofRanked/*nofVisited*/, ranks, summary);
	}

	strus::QueryResult expectedResult_bm25( int maxNofRanks, const Query& query, const strus::StorageClientInterface* storage, bool compacted_ff) const
	{
		std::vector<strus::ResultDocument> ranks;
		std::vector<Document>::const_iterator di = doclist.begin(), de = doclist.end();
		for (; di != de; ++di)
		{
			strus::Index docno = storage->documentNumber( di->docid);
			if (docno)
			{
				double docweight = di->calculate_bm25( statistics, query.features, compacted_ff);
				if (!strus::Math::isequal( docweight, 0.0))
				{
					strus::WeightedDocument wdoc( docno, strus::IndexRange(), docweight);
					std::vector<strus::SummaryElement> summary;
					summary.push_back( strus::SummaryElement( "docid", di->docid, 1.0));
					double collectFeatWeightNorm = 0.0;
					std::vector<int>::const_iterator qi = query.features.begin(), qe = query.features.end();
					for (int qidx=0; qi != qe; ++qi,++qidx)
					{
						double ww = statistics.idf( *qi);
						collectFeatWeightNorm += ww * ww;
						std::vector<strus::Index> pos = di->getFeaturePositions( *qi);
						std::vector<strus::Index>::const_iterator pi = pos.begin(), pe = pos.end();
						for (; pi != pe; ++pi)
						{
							std::string posstr = strus::string_format( "%d", (int)*pi);
							summary.push_back( strus::SummaryElement( "pos", posstr, 1.0, qidx));
						}
					}
					collectFeatWeightNorm = std::sqrt( collectFeatWeightNorm);
					strus::IndexRange contentfield( 1, di->doctreelist.back().field.end());
					std::vector<WeightedPos> wpos = di->getWeightedPos( query.features, contentfield);
					std::vector<WeightedEntity> matches = di->collectWeightedNearMatches( wpos, 0/*dist*/, statistics, g_weightingConfig.maxNofRanks);
					std::vector<WeightedEntity>::const_iterator mi = matches.begin(), me = matches.end();
					for (; mi != me; ++mi)
					{
						summary.push_back( strus::SummaryElement( "match", mi->name, mi->weight / collectFeatWeightNorm));
					}
					ranks.push_back( strus::ResultDocument( wdoc, summary));
				}
			}
		}
		// Cut and sort ranklist:
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
		// Collect populdated summaries:
		std::vector<strus::SummaryElement> summary;
		std::map<std::string,double> matchMap;
		std::vector<strus::ResultDocument>::const_iterator ri = ranks.begin(), re = ranks.end();
		for (; ri != re; ++ri)
		{
			std::vector<strus::SummaryElement>::const_iterator
				si = ri->summaryElements().begin(), se = ri->summaryElements().end();
			for (;si != se; ++si)
			{
				if (si->name() == "match")
				{
					matchMap[ si->value()] += si->weight();
				}
			}
		}
		double maxweight = 0.0;
		std::map<std::string,double>::const_iterator
			mi = matchMap.begin(), me = matchMap.end();
		for (; mi != me; ++mi)
		{
			if (mi->second >maxweight) maxweight = mi->second;
		}
		mi = matchMap.begin();
		for (; mi != me; ++mi)
		{
			summary.push_back( strus::SummaryElement( "match", mi->first, mi->second / maxweight));
		}
		return strus::QueryResult( 0/*evaluationPass*/, nofRanked, nofRanked/*nofVisited*/, ranks, summary);
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

static strus::QueryResult evaluateQuery( strus::QueryEvalInterface* qeval, const strus::StorageClientInterface* storage, const Query& query, int maxNofRanks, const Statistics& statistics)
{
	strus::Reference<strus::QueryInterface> qry( qeval->createQuery( storage));
	if (!qry.get()) throw std::runtime_error( g_errorhnd->fetchError());
	if (statistics.defined())
	{
		strus::GlobalStatistics globstats( statistics.nofDocuments);
		qry->defineGlobalStatistics( globstats);
	}
	std::vector<int>::const_iterator fi = query.features.begin(), fe = query.features.end();
	for (; fi != fe; ++fi)
	{
		std::string word = strus::string_format( "f%d", *fi);
		if (statistics.defined())
		{
			std::map<int,int>::const_iterator gi = statistics.dfmap.find( *fi);
			strus::TermStatistics termstats( gi == statistics.dfmap.end() ? 0 : gi->second);
			qry->defineTermStatistics( "word", word, termstats);
		}
		qry->pushTerm( "word", word, 1);
		qry->defineFeature( "search");
		qry->pushTerm( "word", word, 1);
		qry->defineFeature( "hit");
	}
	return qry->evaluate( 0, maxNofRanks);
}

static bool compareResultAgainstExpected( const strus::ResultDocument& result, const strus::WeightedDocument& expected)
{
	if (result.docno() != expected.docno()) return false;
	if (result.field() != expected.field()) return false;
	return true;
}

static std::string errorMessageResultNotFound(
		const std::map<strus::Index,std::string>& docnoDocidMap, const std::string& testdescr,
		const strus::ResultDocument& result)
{
	std::map<strus::Index,std::string>::const_iterator di = docnoDocidMap.find( result.docno());
	if (di == docnoDocidMap.end()) throw std::runtime_error("undefined document number");

	if (result.field().defined())
	{
		return strus::string_format( "result document %s (%d) field [%d,%d] weight %.5f unexpected match in %s",
						di->second.c_str(), (int)result.docno(),
						(int)result.field().start(), (int)result.field().end(),
						result.weight(), testdescr.c_str());
	}
	else
	{
		return strus::string_format( "result document %s (%d) weight %.5f unexpected match in %s",
						di->second.c_str(), (int)result.docno(),
						result.weight(), testdescr.c_str());
	}
}

static std::string errorMessageExpectedNotFound(
		const std::map<strus::Index,std::string>& docnoDocidMap, const std::string& testdescr,
		const strus::WeightedDocument& expected)
{
	std::map<strus::Index,std::string>::const_iterator di = docnoDocidMap.find( expected.docno());
	if (di == docnoDocidMap.end()) throw std::runtime_error("undefined document number");

	if (expected.field().defined())
	{
		return strus::string_format( "expected document %s (%d) field [%d,%d] weight %.5f not found in %s, ",
						di->second.c_str(), (int)expected.docno(), 
						(int)expected.field().start(), (int)expected.field().end(),
						expected.weight(), testdescr.c_str());
	}
	else
	{
		return strus::string_format( "expected document %s (%d) weight %.5f not found in %s, ",
						di->second.c_str(), (int)expected.docno(), 
						expected.weight(), testdescr.c_str());
	}
}

static std::string descriptionWeightedDocument(
		const std::map<strus::Index,std::string>& docnoDocidMap,
		const strus::WeightedDocument& doc)
{
	std::map<strus::Index,std::string>::const_iterator di = docnoDocidMap.find( doc.docno());
	if (di == docnoDocidMap.end()) throw std::runtime_error("undefined document number");

	if (doc.field().defined())
	{
		return strus::string_format(
			"document %s (%d) field [%d,%d]",
			di->second.c_str(), (int)doc.docno(), (int)doc.field().start(), (int)doc.field().end());
	}
	else
	{
		return strus::string_format(
			"expected document %s (%d)",
			di->second.c_str(), (int)doc.docno());
	}
}

static bool compareSummaryElement( const strus::SummaryElement& s1, const strus::SummaryElement& s2)
{
	if (s1.name() != s2.name()) return false;
	if (s1.value() != s2.value()) return false;
	if (!strus::Math::isequal( s1.weight(), s2.weight(), (double)std::numeric_limits<float>::epsilon())) return false;
	if (s1.index() != s2.index()) return false;
	return true;
}

static bool findSummaryElement( const std::vector<strus::SummaryElement>& haystack, const strus::SummaryElement& needle)
{
	std::vector<strus::SummaryElement>::const_iterator
		hi = haystack.begin(), he = haystack.end();
	for (; hi != he && !compareSummaryElement( needle, *hi); ++hi){}
	return (hi != he);
}

static bool testResultSummaryAgainstExpected(
		const std::vector<strus::SummaryElement>& result,
		const std::vector<strus::SummaryElement>& expected)
{
	if (result.size() != expected.size()) return false;
	{
		std::vector<strus::SummaryElement>::const_iterator
			ei = expected.begin(), ee = expected.end();
		for (; ei != ee && findSummaryElement( result, *ei); ++ei){}
		if (ei != ee) return false;
	}{
		std::vector<strus::SummaryElement>::const_iterator
			ri = result.begin(), re = result.end();
		for (; ri != re && findSummaryElement( expected, *ri); ++ri){}
		if (ri != re) return false;
	}
	return true;
}

static void testResultAgainstExpected(
		const std::map<strus::Index,std::string>& docnoDocidMap, const std::string& testdescr,
		const strus::QueryResult& result, const strus::QueryResult& expected, int ranksChecked)
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
				if (!strus::Math::isequal( ri->weight(), ei->weight(), (double)std::numeric_limits<float>::epsilon()))
				{
					std::string descr = descriptionWeightedDocument( docnoDocidMap, *ri);
					throw strus::runtime_error(
						"weight %.7f of result '%s' of '%s' does not match expected %.7f",
						ri->weight(), descr.c_str(), testdescr.c_str(), ei->weight());
				}
				if (!testResultSummaryAgainstExpected( ri->summaryElements(), ei->summaryElements()))
				{
					std::string descr = descriptionWeightedDocument( docnoDocidMap, *ri);
					throw strus::runtime_error(
						"summary elements of result '%s' of '%s' do not match expected",
						descr.c_str(), testdescr.c_str());
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
				if (!strus::Math::isequal( ri->weight(), ei->weight(), (double)std::numeric_limits<float>::epsilon()))
				{
					std::string descr = descriptionWeightedDocument( docnoDocidMap, *ri);
					throw strus::runtime_error(
						"weight %.7f of result '%s' of '%s' does not match expected %.7f",
						ri->weight(), descr.c_str(), testdescr.c_str(), ei->weight());
				}
				if (!testResultSummaryAgainstExpected( ri->summaryElements(), ei->summaryElements()))
				{
					std::string descr = descriptionWeightedDocument( docnoDocidMap, *ri);
					throw strus::runtime_error(
						"summary elements of result '%s' of '%s' do not match expected",
						descr.c_str(), testdescr.c_str());
				}
			}
		}
	}{
		if (!testResultSummaryAgainstExpected( result.summaryElements(), expected.summaryElements()))
		{
			throw strus::runtime_error( "result summary elements of '%s' do not match expected", testdescr.c_str());
		}
	}
}

static void checkResult( const std::map<strus::Index,std::string>& docnoDocidMap, int qryidx, const Query& qry, const strus::QueryResult& result, const strus::QueryResult& expected, int ranksChecked)
{
	std::string qrystr = qry.tostring();
	std::string testdescr = strus::string_format( "test query %d = '%s'", qryidx, qrystr.c_str());
	testResultAgainstExpected( docnoDocidMap, testdescr, result, expected, ranksChecked);
}

static void printSummary( std::ostream& out, const std::vector<strus::SummaryElement>& summaryElements, const char* indentstr)
{
	std::vector<strus::SummaryElement>::const_iterator
		si = summaryElements.begin(),
		se = summaryElements.end();
	for (; si != se; ++si)
	{
		if (si->index() >= 0)
		{
			out << strus::string_format( "%s%s = '%s' %.5f %d",
					indentstr, si->name().c_str(), si->value().c_str(), si->weight(), si->index())
				<< std::endl;
		}
		else
		{
			out << strus::string_format( "%s%s = %s %.5f",
					indentstr, si->name().c_str(), si->value().c_str(), si->weight())
				<< std::endl;
		}
	}
}

static void printResult( std::ostream& out, const std::map<strus::Index,std::string>& docnoDocidMap, const strus::QueryResult& result)
{
	std::vector<strus::ResultDocument>::const_iterator ri = result.ranks().begin(), re = result.ranks().end();
	for (int ridx=1; ri != re; ++ri,++ridx)
	{
		std::map<strus::Index,std::string>::const_iterator di = docnoDocidMap.find( ri->docno());
		if (di == docnoDocidMap.end()) throw std::runtime_error( strus::string_format( "unknown document number in result %d", ri->docno()));
		if (ri->field().defined())
		{
			out << strus::string_format( "rank %d: %s [%d,%d] %.5f",
				ridx, di->second.c_str(),
				(int)ri->field().start(), (int)ri->field().end(),
				ri->weight())
			<< std::endl;
		}
		else
		{
			out << strus::string_format( "rank %d: %s %.5f",
				ridx, di->second.c_str(),
				ri->weight())
			<< std::endl;
		}
		printSummary( out, ri->summaryElements(), "\t> ");
	}
	printSummary( out, result.summaryElements(), "summary ");
}

static strus::Reference<strus::QueryEvalInterface> queryEval_bm25pff( strus::QueryProcessorInterface* queryproc)
{
	strus::Reference<strus::QueryEvalInterface> qeval( strus::createQueryEval( g_errorhnd));
	if (!qeval.get()) throw std::runtime_error( g_errorhnd->fetchError());

	const strus::WeightingFunctionInterface* weightingBm25pff = queryproc->getWeightingFunction( "bm25pff");
	if (!weightingBm25pff) throw std::runtime_error( "undefined weighting function 'bm25pff'");
	strus::Reference<strus::WeightingFunctionInstanceInterface> wfunc( weightingBm25pff->createInstance( queryproc));
	if (!wfunc.get()) throw std::runtime_error( g_errorhnd->fetchError());
	wfunc->addNumericParameter( "results", strus::NumericVariant::asint( g_weightingConfig.maxNofRanks));
	wfunc->addNumericParameter( "b", strus::NumericVariant::asdouble( g_weightingConfig.b));
	wfunc->addNumericParameter( "k1", strus::NumericVariant::asdouble( g_weightingConfig.k1));
	wfunc->addNumericParameter( "avgdoclen", strus::NumericVariant::asint( g_weightingConfig.avgDocLength));
	wfunc->addStringParameter( "metadata_doclen", "doclen");
	wfunc->addStringParameter( "struct", "title");
	wfunc->addNumericParameter( "maxdf", strus::NumericVariant::asdouble( g_weightingConfig.maxdf));
	wfunc->addNumericParameter( "dist_imm", strus::NumericVariant::asint( g_weightingConfig.distance_imm));
	wfunc->addNumericParameter( "dist_close", strus::NumericVariant::asint( g_weightingConfig.distance_close));
	wfunc->addNumericParameter( "dist_near", strus::NumericVariant::asint( g_weightingConfig.distance_near));
	wfunc->addNumericParameter( "cluster", strus::NumericVariant::asdouble( g_weightingConfig.minClusterSize));
	wfunc->addNumericParameter( "hotspots", strus::NumericVariant::asint( g_weightingConfig.maxNofRanks));
	wfunc->addNumericParameter( "ffbase", strus::NumericVariant::asdouble( g_weightingConfig.minFfWeight));

	const strus::SummarizerFunctionInterface* summarizerAttribute = queryproc->getSummarizerFunction( "attribute");
	if (!summarizerAttribute) throw std::runtime_error( "undefined summarizer function 'attribute'");
	strus::Reference<strus::SummarizerFunctionInstanceInterface> attributefunc( summarizerAttribute->createInstance( queryproc));
	if (!attributefunc.get()) throw std::runtime_error( g_errorhnd->fetchError());
	attributefunc->addStringParameter( "name", "docid");

	const strus::SummarizerFunctionInterface* summarizerStructHeader = queryproc->getSummarizerFunction( "structheader");
	if (!summarizerStructHeader) throw std::runtime_error( "undefined summarizer function 'structheader'");
	strus::Reference<strus::SummarizerFunctionInstanceInterface> headerfunc( summarizerStructHeader->createInstance( queryproc));
	if (!headerfunc.get()) throw std::runtime_error( g_errorhnd->fetchError());
	headerfunc->addStringParameter( "text", "orig");
	if (g_random.get(0,2)==0) headerfunc->addStringParameter( "struct", "title");

	const strus::SummarizerFunctionInterface* summarizerPhrase = queryproc->getSummarizerFunction( "matchphrase");
	if (!summarizerPhrase) throw std::runtime_error( "undefined summarizer function 'matchphrase'");
	strus::Reference<strus::SummarizerFunctionInstanceInterface> phrasefunc( summarizerPhrase->createInstance( queryproc));
	if (!phrasefunc.get()) throw std::runtime_error( g_errorhnd->fetchError());
	phrasefunc->addStringParameter( "text", "orig");
	phrasefunc->addStringParameter( "entity", "entity");
	phrasefunc->addNumericParameter( "maxdf", strus::NumericVariant::asdouble( g_weightingConfig.maxdf));
	phrasefunc->addNumericParameter( "sentences", strus::NumericVariant::asint( g_weightingConfig.nofSummarySentences));
	phrasefunc->addNumericParameter( "dist_imm", strus::NumericVariant::asint( g_weightingConfig.distance_imm));
	phrasefunc->addNumericParameter( "dist_close", strus::NumericVariant::asint( g_weightingConfig.distance_close));
	phrasefunc->addNumericParameter( "dist_near", strus::NumericVariant::asint( g_weightingConfig.distance_near));
	phrasefunc->addNumericParameter( "dist_sentence", strus::NumericVariant::asint( g_weightingConfig.maxNofSummarySentenceWords));
	phrasefunc->addNumericParameter( "cluster", strus::NumericVariant::asdouble( g_weightingConfig.minClusterSize));

	std::vector<strus::QueryEvalInterface::FeatureParameter> noParam;
	std::vector<strus::QueryEvalInterface::FeatureParameter> weightingParam;
	weightingParam.push_back( strus::QueryEvalInterface::FeatureParameter( "match", "search"));
	weightingParam.push_back( strus::QueryEvalInterface::FeatureParameter( "punct", "punct"));

	qeval->addSelectionFeature( "search");
	qeval->addTerm( "punct", "eos", "");
	qeval->addWeightingFunction( wfunc.release(), weightingParam);
	qeval->addSummarizerFunction( "docid", attributefunc.release(), noParam);
	qeval->addSummarizerFunction( "header", headerfunc.release(), noParam);
	qeval->addSummarizerFunction( "phrase", phrasefunc.release(), weightingParam);

	if (g_errorhnd->hasError())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
	return qeval;
}

static strus::Reference<strus::QueryEvalInterface> queryEval_bm25( strus::QueryProcessorInterface* queryproc)
{
	strus::Reference<strus::QueryEvalInterface> qeval( strus::createQueryEval( g_errorhnd));
	if (!qeval.get()) throw std::runtime_error( g_errorhnd->fetchError());

	const strus::WeightingFunctionInterface* weightingBm25 = queryproc->getWeightingFunction( "bm25");
	if (!weightingBm25) throw std::runtime_error( "undefined weighting function 'bm25'");
	strus::Reference<strus::WeightingFunctionInstanceInterface> wfunc( weightingBm25->createInstance( queryproc));
	if (!wfunc.get()) throw std::runtime_error( g_errorhnd->fetchError());
	wfunc->addNumericParameter( "b", strus::NumericVariant::asdouble( g_weightingConfig.b));
	wfunc->addNumericParameter( "k1", strus::NumericVariant::asdouble( g_weightingConfig.k1));
	wfunc->addNumericParameter( "avgdoclen", strus::NumericVariant::asint( g_weightingConfig.avgDocLength));
	wfunc->addStringParameter( "metadata_doclen", "doclen");

	const strus::SummarizerFunctionInterface* summarizerAttribute = queryproc->getSummarizerFunction( "attribute");
	if (!summarizerAttribute) throw std::runtime_error( "undefined summarizer function 'attribute'");
	strus::Reference<strus::SummarizerFunctionInstanceInterface> attributefunc( summarizerAttribute->createInstance( queryproc));
	if (!attributefunc.get()) throw std::runtime_error( g_errorhnd->fetchError());
	attributefunc->addStringParameter( "name", "docid");

	const strus::SummarizerFunctionInterface* summarizerPositions = queryproc->getSummarizerFunction( "listmatch");
	if (!summarizerPositions) throw std::runtime_error( "undefined summarizer function 'matchpos'");
	strus::Reference<strus::SummarizerFunctionInstanceInterface> posfunc( summarizerPositions->createInstance( queryproc));
	if (!posfunc.get()) throw std::runtime_error( g_errorhnd->fetchError());
	posfunc->addNumericParameter( "results", strus::NumericVariant::asint( g_weightingConfig.maxNofRanks));
	posfunc->addStringParameter( "fmt", "{pos}");
	posfunc->addStringParameter( "tag", "tag");
	posfunc->addStringParameter( "text", "orig");

	const strus::SummarizerFunctionInterface* summarizerAccuNear = queryproc->getSummarizerFunction( "accunear");
	if (!summarizerAccuNear) throw std::runtime_error( "undefined summarizer function 'accunear'");
	strus::Reference<strus::SummarizerFunctionInstanceInterface> matchfunc( summarizerAccuNear->createInstance( queryproc));
	if (!matchfunc.get()) throw std::runtime_error( g_errorhnd->fetchError());
	matchfunc->addStringParameter( "collect", "tag=tag;sep='#';type=entity,orig");
	matchfunc->addNumericParameter( "results", strus::NumericVariant::asint( g_weightingConfig.maxNofRanks));
	matchfunc->addNumericParameter( "maxdf", strus::NumericVariant::asdouble( g_weightingConfig.maxdf));
	matchfunc->addNumericParameter( "dist_imm", strus::NumericVariant::asint( g_weightingConfig.distance_imm));
	matchfunc->addNumericParameter( "dist_close", strus::NumericVariant::asint( g_weightingConfig.distance_close));
	matchfunc->addNumericParameter( "dist_near", strus::NumericVariant::asint( g_weightingConfig.distance_near));
	matchfunc->addNumericParameter( "dist_collect", strus::NumericVariant::asint( 0/*only collect at matching position*/));
	matchfunc->addNumericParameter( "cluster", strus::NumericVariant::asdouble( g_weightingConfig.minClusterSize));

	std::vector<strus::QueryEvalInterface::FeatureParameter> noParam;
	std::vector<strus::QueryEvalInterface::FeatureParameter> weightingParam;
	weightingParam.push_back( strus::QueryEvalInterface::FeatureParameter( "match", "search"));
	std::vector<strus::QueryEvalInterface::FeatureParameter> posParam;
	posParam.push_back( strus::QueryEvalInterface::FeatureParameter( "match", "hit"));
	std::vector<strus::QueryEvalInterface::FeatureParameter> matchParam;
	matchParam.push_back( strus::QueryEvalInterface::FeatureParameter( "match", "hit"));
	matchParam.push_back( strus::QueryEvalInterface::FeatureParameter( "punct", "punct"));

	qeval->addSelectionFeature( "search");
	qeval->addTerm( "punct", "eos", "");
	qeval->usePositionInformation( "search", false);
	qeval->usePositionInformation( "hit", true);
	qeval->addWeightingFunction( wfunc.release(), weightingParam);
	qeval->addSummarizerFunction( "docid", attributefunc.release(), noParam);
	qeval->addSummarizerFunction( "pos", posfunc.release(), posParam);
	qeval->addSummarizerFunction( "match", matchfunc.release(), matchParam);

	if (g_errorhnd->hasError())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
	return qeval;
}

static void insertCollection( Collection& collection, Storage& storage, std::map<strus::Index,std::string>& docnoDocidMap, const std::string& selectDocid, int commitSize)
{
	if (selectDocid.empty())
	{
		collection.insert( storage.sci.get(), commitSize);
		docnoDocidMap = collection.docnoDocidMap( storage.sci.get());

		collection.check( storage.sci.get());
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

		collection.check( storage.sci.get(), selectDocid);
	}
}

static void runQueryAndVerifyResult( strus::QueryEvalInterface* qeval, const strus::StorageClientInterface* storage, int queryIdx, const Query& qry, const strus::QueryResult& expected, const std::map<strus::Index,std::string>& docnoDocidMap, const Statistics& statistics)
{
	int nofRanksChecked = g_weightingConfig.maxNofRanks / 10;

	std::string qrystr = qry.tostring();
	if (g_verbosity >= 1)
	{
		std::cerr << strus::string_format( "evaluate query %d: %s", queryIdx, qrystr.c_str()) << std::endl;
	}
	strus::QueryResult result = evaluateQuery( qeval, storage, qry, g_weightingConfig.maxNofRanks, statistics);
	if (result.ranks().empty() && g_errorhnd->hasError())
	{
		throw std::runtime_error( g_errorhnd->fetchError());
	}
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
	checkResult( docnoDocidMap, queryIdx, qry, result, expected, nofRanksChecked);
}

enum QueryEvalMethod {
	BM25PFF,
	BM25
};

static void runRandomQueriesAndVerifyResult( QueryEvalMethod method, strus::QueryEvalInterface* qeval, const strus::StorageClientInterface* storage, const std::map<strus::Index,std::string>& docnoDocidMap, const Statistics& statistics, const Collection& collection, int selectQuery, int nofQueries)
{
	std::vector<Query> queries = collection.randomQueries( nofQueries);
	if (selectQuery >= 0)
	{
		if (selectQuery >= (int)queries.size())
		{
			throw std::runtime_error("selected query undefined");
		}
		const Query& qry = queries[ selectQuery];
		strus::QueryResult expected;
		switch (method)
		{
			case BM25PFF: expected = collection.expectedResult_bm25pff( g_weightingConfig.maxNofRanks, qry, storage); break;
			case BM25: expected = collection.expectedResult_bm25( g_weightingConfig.maxNofRanks, qry, storage, true/*compacted_ff*/); break;
		}
		runQueryAndVerifyResult( qeval, storage, selectQuery, qry, expected, docnoDocidMap, statistics);
	}
	else
	{
		std::vector<Query>::const_iterator qi = queries.begin(), qe = queries.end();
		for (int qidx=0;qi != qe; ++qi,++qidx)
		{
			strus::QueryResult expected;
			switch (method)
			{
				case BM25PFF: expected = collection.expectedResult_bm25pff( g_weightingConfig.maxNofRanks, *qi, storage); break;
				case BM25: expected = collection.expectedResult_bm25( g_weightingConfig.maxNofRanks, *qi, storage, true/*compacted_ff*/); break;
			}
			runQueryAndVerifyResult( qeval, storage, qidx, *qi, expected, docnoDocidMap, statistics);
		}
	}
}

static void testWeighting( QueryEvalMethod method, int nofDocuments, int nofTerms, int nofNodes, int commitSize, int nofQueries, const std::string& selectDocid, int selectQuery)
{
	Storage storage( g_fileLocator, g_errorhnd);
	storage.open( "path=storage", true);
	strus::test::Storage::MetaDataDef metadata[] = {{"doclen", "UINT16"},{0,0}};
	storage.defineMetaData( metadata);

	Collection collection( nofDocuments, nofTerms, nofNodes);

	strus::Reference<strus::QueryProcessorInterface> queryproc( strus::createQueryProcessor( g_fileLocator, g_errorhnd));
	if (!queryproc.get()) throw std::runtime_error( g_errorhnd->fetchError());
	strus::Reference<strus::QueryEvalInterface> qeval;
	switch (method)
	{
		case BM25PFF: qeval = queryEval_bm25pff( queryproc.get()); break;
		case BM25: qeval = queryEval_bm25( queryproc.get()); break;
	}
	bool useExternStats = (1 == g_random.get( 0, 2)) || !selectDocid.empty();
	if (g_verbosity >= 1)
	{
		if (useExternStats) std::cerr << "use external statistics" << std::endl;
	}
	std::map<strus::Index,std::string> docnoDocidMap;
	insertCollection( collection, storage, docnoDocidMap, selectDocid, commitSize);

	runRandomQueriesAndVerifyResult(
		method, qeval.get(), storage.sci.get(), docnoDocidMap,
		useExternStats ? collection.statistics : Statistics(),
		collection, selectQuery, nofQueries); 
}


static void printUsage()
{
	std::cerr << "usage: testWeighting [options] <method> <nofdocs> <nofterms> <nofnodes> <commitsize> <nofqry>" << std::endl;
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
	std::cerr << "<method>         :retrieval method to test {'bm25pff'}" << std::endl;
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
		if (argi + 7 > argc)
		{
			printUsage();
			throw std::runtime_error( "too few arguments");
		}
		if (argi + 7 < argc)
		{
			printUsage();
			throw std::runtime_error( "too many arguments");
		}
		int ai = 0;
		std::string testType = argv[ argi+ai];
		++ai;
		int nofDocuments = strus::numstring_conv::toint( argv[ argi+ai], std::strlen(argv[ argi+ai]), std::numeric_limits<int>::max());
		++ai;
		int nofTerms = strus::numstring_conv::toint( argv[ argi+ai], std::strlen(argv[ argi+ai]), std::numeric_limits<int>::max());
		++ai;
		int nofNodes = strus::numstring_conv::toint( argv[ argi+ai], std::strlen(argv[ argi+ai]), std::numeric_limits<int>::max());
		++ai;
		int commitSize = strus::numstring_conv::toint( argv[ argi+ai], std::strlen(argv[ argi+ai]), std::numeric_limits<int>::max());
		++ai;
		g_weightingConfig.maxNofRanks = strus::numstring_conv::toint( argv[ argi+ai], std::strlen(argv[ argi+ai]), std::numeric_limits<int>::max());
		++ai;
		int nofQueries = strus::numstring_conv::toint( argv[ argi+ai], std::strlen(argv[ argi+ai]), std::numeric_limits<int>::max());
		++ai;

		if (g_verbosity >= 1)
		{
			std::cerr << strus::string_format("test %s using random seed %d", testType.c_str(), g_random.seed()) << std::endl;
		}
		QueryEvalMethod method = BM25;
		if (testType == "bm25pff")
		{
			method = BM25PFF;
		}
		else if (testType == "bm25")
		{
			method = BM25;
		}
		else
		{
			throw std::runtime_error(strus::string_format( "unknown test retrieval type '%s'", testType.c_str()));
		}
		testWeighting( method, nofDocuments, nofTerms, nofNodes, commitSize, nofQueries, selectDocid, selectQuery);

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


