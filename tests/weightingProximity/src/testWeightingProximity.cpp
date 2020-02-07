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
#include "strus/structIteratorInterface.hpp"
#include "strus/queryInterface.hpp"
#include "strus/queryEvalInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/summarizerFunctionInstanceInterface.hpp"
#include "strus/weightedField.hpp"
#include "strus/weightedDocument.hpp"
#include "strus/resultDocument.hpp"
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
		result.append( strus::string_format( "[%d,%d]", (int)field.start(), (int)field.end()));
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
	if (startpos <= rt.field.start() || startpos >= strus::Constants::storage_max_position_info()) throw std::runtime_error(_TXT("test document tree too complex"));
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
				for (; si != se; ++si,++ci){}
				return createRandomQuery( *ci);
			}
		}
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

static void fillStructureList( std::vector<strus::test::StructureDef>& structurelist, const DocTreeNode& node)
{
	strus::IndexRange header( node.field.start(), node.field.start() + node.titlear.size());
	strus::IndexRange content( header.end(), header.end() + node.contentar.size());
	if (header.defined() && content.defined())
	{
		structurelist.push_back( strus::test::StructureDef( "title", header, content));
	}
	std::list<DocTreeNode>::const_iterator ci = node.chld.begin(), ce = node.chld.end();
	for (; ci != ce; ++ci)
	{
		structurelist.push_back( strus::test::StructureDef( "title", header, ci->field));
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
		bool operator()( const FeaturePos& aa, strus::Index pos) const
		{
			return aa.pos < pos;
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
			titletext.append( strus::string_format( "F%d", *fi));
		}
		if (!titletext.empty()) titlelist.push_back( LabeledTitle( titletext, depth));
		std::list<DocTreeNode>::const_iterator ci = node.chld.begin(), ce = node.chld.end();
		for (; ci != ce; ++ci)
		{
			fillTitles( titlelist, *ci, field, depth+1);
		}
	}
}

static void fillFeatureList( std::vector<strus::test::Feature>& featurelist, strus::Index startpos, const std::vector<int>& featar)
{
	std::vector<int>::const_iterator fi = featar.begin(), fe = featar.end();
	for (int fidx=0; fi != fe; ++fi,++fidx)
	{
		strus::Index pos = startpos + fidx;
		if (*fi == 0/*EOS*/)
		{
			featurelist.push_back( strus::test::Feature( strus::test::Feature::SearchIndex, "punct", "", pos));
			featurelist.push_back( strus::test::Feature( strus::test::Feature::ForwardIndex, "orig", ".", pos));
		}
		else
		{
			std::string word = strus::string_format( "f%d", *fi);
			featurelist.push_back( strus::test::Feature( strus::test::Feature::SearchIndex, "word", word, pos));
			std::string orig = strus::string_format( "F%d", *fi);
			featurelist.push_back( strus::test::Feature( strus::test::Feature::ForwardIndex, "orig", orig, pos));
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
	float minClusterSize;
	int nofSummarySentences;
	int maxNofSummarySentenceWords;
	double minFfWeight;
	int avgDocLength;
	double b;
	double k1;
	double maxdf;
};
static WeightingConfig g_weightingConfig = {
	2/*distance_imm*/,
	8/*distance_close*/,
	40/*distance_near*/,
	0.0/*minClusterSize*/,
	1/*nofSummarySentences*/,
	100/*maxNofSummarySentenceWords*/,
	0.1/*minFfWeight*/,
	100/*avgDocLength*/,
	0.75/*b*/,
	1.5/*k1*/,
	1.0/*maxdf*/
};

struct WeightedPos
{
	int featidx;
	strus::Index pos;
	double ff;

	WeightedPos( int featidx_, strus::Index pos_, double ff_)
		:featidx(featidx_),pos(pos_),ff(ff_){}
	WeightedPos( const WeightedPos& o)
		:featidx(o.featidx),pos(o.pos),ff(o.ff){}
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
		doc->setMetaData( "doclen", strus::NumericVariant::asint( doctreelist.empty() ? 0 : doctreelist.back().field.end()));

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
			if (dist_imm_cnt + dist_close_cnt + dist_sent_cnt + dist_near_cnt + dist_title_cnt >= minClusterSize)
			{
				return (1.0 - minFfWeight) * (ww / 4) + minFfWeight;
			}
			else
			{
				return 0.0;
			}
		}
	};

	void collectWeightedPos( 
			std::vector<WeightedPos>& res, 
			const std::vector<int>& features,
			const strus::IndexRange& titlefield, const strus::IndexRange& contentfield) const
	{
		strus::SkipScanArray<FeaturePos,strus::Index,FeaturePos::FindPosCompare>
			ar( featposlist.data(), featposlist.size());
		int startidx = ar.upperbound( contentfield.start());
		int endidx = ar.upperbound( contentfield.end());
		if (startidx < 0) return;
		if (endidx < 0) endidx = featposlist.size();

		std::vector<FeaturePos>::const_iterator
			fi = featposlist.begin() + startidx,
			fe = featposlist.begin() + endidx;
		for (; fi != fe; ++fi)
		{
			std::vector<int>::const_iterator qi = std::find( features.begin(), features.end(), fi->featidx);
			if (qi != features.end())
			{
				std::vector<int> othfeats = features;
				othfeats.erase( othfeats.begin() + (qi - features.begin()));
				std::set<int> titlefeats = getTitleFeatures( titlefield, othfeats);

				int minClusterSize = g_weightingConfig.minClusterSize * features.size() + 0.5;
				int weightedNeighbours = features.size()-1;
				TouchCounter touchCounter( weightedNeighbours, minClusterSize, g_weightingConfig.minFfWeight);

				std::vector<int>::const_iterator
					oi = othfeats.begin(), oe = othfeats.end();
				for (; oi != oe; ++oi)
				{
					bool gotMatch = false;
					{
						std::vector<FeaturePos>::const_iterator prev_fi = fi;
						if (prev_fi != featposlist.begin())
						{
							bool gotEos = false;
							for (--prev_fi; prev_fi >= featposlist.begin(); --prev_fi)
							{
								if (prev_fi->featidx == 0/*EOS*/)
								{
									gotEos = true;
									continue;
								}
								if (*oi == prev_fi->featidx)
								{
									int dist = fi->pos - prev_fi->pos;
									if (!gotEos && dist <= g_weightingConfig.distance_imm)
									{
										touchCounter.dist_imm_cnt++;
										gotMatch = true;
										break;
									}
									else if (!gotEos && dist <= g_weightingConfig.distance_close)
									{
										touchCounter.dist_close_cnt++;
										gotMatch = true;
										break;
									}
									else if (!gotEos && dist <= g_weightingConfig.maxNofSummarySentenceWords)
									{
										touchCounter.dist_sent_cnt++;
										gotMatch = true;
										break;
									}
								}
							}
						}
					}{
						if (!gotMatch)
						{
							std::vector<FeaturePos>::const_iterator next_fi = fi;
							bool gotEos = false;
							for (++next_fi; next_fi != featposlist.end(); ++next_fi)
							{
								if (next_fi->featidx == 0/*EOS*/)
								{
									gotEos = true;
									continue;
								}
								if (*oi == next_fi->featidx)
								{
									int dist = next_fi->pos - fi->pos - 1/*feature length*/;
									if (!gotEos && dist <= g_weightingConfig.distance_imm)
									{
										touchCounter.dist_imm_cnt++;
										gotMatch = true;
										break;
									}
									else if (!gotEos && dist <= g_weightingConfig.distance_close)
									{
										touchCounter.dist_close_cnt++;
										gotMatch = true;
										break;
									}
									else if (!gotEos && dist <= g_weightingConfig.maxNofSummarySentenceWords)
									{
										touchCounter.dist_sent_cnt++;
										gotMatch = true;
										break;
									}
								}
							}
						}
					}{
						if (!gotMatch)
						{
							if (fi != featposlist.begin())
							{
								std::vector<FeaturePos>::const_iterator prev_fi = fi;
								for (--prev_fi; prev_fi >= featposlist.begin(); --prev_fi)
								{
									if (*oi == prev_fi->featidx)
									{
										int dist = fi->pos - prev_fi->pos;
										if (dist <= g_weightingConfig.distance_near)
										{
											touchCounter.dist_near_cnt++;
											gotMatch = true;
											break;
										}
									}
								}
							}
						}
						if (!gotMatch)
						{
							std::vector<FeaturePos>::const_iterator next_fi = fi;
							for (++next_fi; next_fi != featposlist.end(); ++next_fi)
							{
								if (*oi == next_fi->featidx)
								{
									int dist = next_fi->pos - fi->pos - 1/*feature length*/;
									if (dist <= g_weightingConfig.distance_near)
									{
										touchCounter.dist_near_cnt++;
										gotMatch = true;
										break;
									}
								}
							}
						}
						if (!gotMatch && titlefeats.find( fi->featidx) != titlefeats.end())
						{
							touchCounter.dist_title_cnt++;
							gotMatch = true;
						}
					}
				}
				res.push_back( WeightedPos( fi->featidx, fi->pos, touchCounter.weight()));
			}
		}
	}

	std::vector<WeightedPos> getWeightedPos( const std::vector<int>& features, const DocTreeNode& tree) const
	{
		std::vector<WeightedPos> rt;
		strus::IndexRange titlefield( tree.field.start(), tree.field.start() + tree.titlear.size());
		strus::IndexRange contentfield( tree.field.start(), tree.field.start() + tree.titlear.size());
		collectWeightedPos( rt, features, titlefield, contentfield);
		return rt;
	}

	static double postingsWeight_bm25ff( int collectionNofDocs, const std::map<int,int>& collectionDfMap, int doclen, int featidx, double ff)
	{
		std::map<int,int>::const_iterator ni = collectionDfMap.find( featidx);
		if (ni == collectionDfMap.end()) return 0.0;
		int df = ni->second;
		double b = g_weightingConfig.b;
		double k1 = g_weightingConfig.k1;
		double idf = strus::Math::log10( (collectionNofDocs - df + 0.5) / (df + 0.5));
		if (idf < 0.00001)
		{
			//... avoid negative IDFs and to small idfs
			idf = 0.00001;
		}
		double featureWeight = idf;
		if (b)
		{
			double rel_doclen = (double)doclen / g_weightingConfig.avgDocLength;
			return featureWeight
					* (ff * (k1 + 1.0))
					/ (ff + k1
						* (1.0 - b + b * rel_doclen));
		}
		else
		{
			return featureWeight
					* (ff * (k1 + 1.0))
					/ (ff + k1 * 1.0);
		}
	}

	void collectRanklists_bm25ff( std::vector<strus::WeightedField>& res, int collectionNofDocs, const std::map<int,int>& collectionDfMap, const std::vector<int>& queryFeatures, const DocTreeNode& node) const
	{
		!!!! wpos should be calculated global per document because touches go accross boundaries
		std::vector<WeightedPos> wpos = getWeightedPos( queryFeatures, node);
		std::vector<WeightedPos>::const_iterator wi = wpos.begin(), we = wpos.end();
		std::map<int,double> featFfMap;
		for (; wi != we; ++wi)
		{
			featFfMap[ wi->featidx] += wi->ff;
		}
		double ww;
		std::vector<int>::const_iterator qi = queryFeatures.begin(), qe = queryFeatures.end();
		for (; qi != qe; ++qi)
		{
			int doclen = node.field.end() - node.field.start();
			std::map<int,double>::const_iterator fi = featFfMap.find( *qi);
			double ff = fi == featFfMap.end() ? 0.0 : fi->second;
			ww += postingsWeight_bm25ff( collectionNofDocs, collectionDfMap, doclen, *qi, ff);
		}
		std::list<DocTreeNode>::const_iterator ci = node.chld.begin(), ce = node.chld.end();
		for (; ci != ce; ++ci)
		{
			collectRanklists_bm25ff( res, collectionNofDocs, collectionDfMap, queryFeatures, *ci);
		}
		if (ww > 0.0)
		{
			res.push_back( strus::WeightedField( node.field, ww));
		}
		!!!! ACCUMULATE ALSO CHILDREN WEIGHTS 
	}

	std::vector<strus::WeightedField> calculateRanklist_bm25ff( int collectionNofDocs, const std::map<int,int>& collectionDfMap, const std::vector<int>& queryFeatures, const DocTreeNode& doctree) const
	{
		std::vector<strus::WeightedField> rt;
		collectRanklists_bm25ff( rt, collectionNofDocs, collectionDfMap, queryFeatures, doctree);
		return rt;
	}

};

struct Collection
{
	std::vector<Document> doclist;
	std::map<int,int> dfmap;

	Collection( int nofDocuments, int nofTerms, int nofNodes)
		:doclist(),dfmap()
	{
		int di = 0, de = nofDocuments;
		for (; di != de; ++di)
		{
			doclist.push_back( Document::createRandom( di, nofTerms, nofNodes));
			doclist.back().collectDfStats( dfmap);
		}
		
	}
	Collection( const Collection& o)
		:doclist(o.doclist),dfmap(o.dfmap){}

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
		const DocTreeNode& tree = doc.doctreelist[ g_random.get( 0, doc.doctreelist.size())];
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

	strus::QueryResult expectedResult_bm25ff( int maxNofRanks, const Query& query, strus::StorageClientInterface* storage) const
	{
		std::vector<strus::ResultDocument> ranks;
		std::vector<Document>::const_iterator di = doclist.begin(), de = doclist.end();
		for (; di != de; ++di)
		{
			strus::Index docno = storage->documentNumber( di->docid);
			if (docno)
			{
				std::vector<DocTreeNode>::const_iterator
					ti = di->doctreelist.begin(), te = di->doctreelist.end();
				for  (; ti != te; ++ti)
				{
					std::vector<strus::WeightedField>
						weightedFields = di->calculateRanklist_bm25ff( doclist.size(), dfmap, query.features, *ti);
					std::vector<strus::WeightedField>::const_iterator
						wi = weightedFields.begin(), we = weightedFields.end();
					for (; wi != we; ++wi)
					{
						strus::WeightedDocument wdoc( docno, *wi);
						std::vector<strus::SummaryElement> summary;
						summary.push_back( strus::SummaryElement( "docid", di->docid, 1.0));

						std::vector<LabeledTitle> titlelist;
						fillTitles( titlelist, *ti, wi->field(), 0/*depth*/);
						std::vector<LabeledTitle>::const_iterator
							li = titlelist.begin(), le = titlelist.end();
						for (int lidx=0; li != le; ++li,++lidx)
						{
							summary.push_back( strus::SummaryElement( "header", li->text, 1.0, li->hierarchy));
						}
						ranks.push_back( strus::ResultDocument( wdoc, summary));
					}
				}
				!!!!! HIE WIITER: CALCULATE ALSO TOTAL DOCUMENT RANKLIST
			}
		}
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
	for (; ei != ee; ++ri,++ei)
	{
		if (ri->name() != ei->name()) return false;
		if (ri->value() != ei->value()) return false;
		if (!strus::Math::isequal( ri->weight(), ei->weight())) return false;
		if (ri->index() != ei->index()) return false;
	}
	return true;
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

static void testWeighting_bm25pff( int nofDocuments, int nofTerms, int nofNodes, int commitSize, int nofQueryies, const std::string& selectDocid, int selectQuery)
{
	Storage storage( g_fileLocator, g_errorhnd);
	storage.open( "path=storage", true);
	strus::test::Storage::MetaDataDef metadata[] = {{"doclen", "UINT16"},{0,0}};
	storage.defineMetaData( metadata);

	Collection collection( nofDocuments, nofTerms, nofNodes);

	double hierarchyWeightFactor = ((double)g_random.get( 0, 100000) / 100000.0) * 0.3 + 0.7;
	int maxNofRanks = 200;

	strus::Reference<strus::QueryEvalInterface> qeval( strus::createQueryEval( g_errorhnd));
	if (!qeval.get()) throw std::runtime_error( g_errorhnd->fetchError());
	strus::Reference<strus::QueryProcessorInterface> queryproc( strus::createQueryProcessor( g_fileLocator, g_errorhnd));
	if (!queryproc.get()) throw std::runtime_error( g_errorhnd->fetchError());

	const strus::WeightingFunctionInterface* weightingBm25pff = queryproc->getWeightingFunction( "bm25pff");
	if (!weightingBm25pff) throw std::runtime_error( "undefined weighting function 'bm25pff'");
	strus::Reference<strus::WeightingFunctionInstanceInterface> wfunc( weightingBm25pff->createInstance( queryproc.get()));
	if (!wfunc.get()) throw std::runtime_error( g_errorhnd->fetchError());
	wfunc->addNumericParameter( "results", strus::NumericVariant::asint( maxNofRanks));
	wfunc->addNumericParameter( "b", strus::NumericVariant::asdouble( g_weightingConfig.b));
	wfunc->addNumericParameter( "k1", strus::NumericVariant::asdouble( g_weightingConfig.k1));
	wfunc->addNumericParameter( "avgdoclen", strus::NumericVariant::asint( g_weightingConfig.avgDocLength));
	wfunc->addStringParameter( "metadata_doclen", "doclen");
	wfunc->addStringParameter( "struct", "title");
	wfunc->addNumericParameter( "maxdf", strus::NumericVariant::asdouble( g_weightingConfig.maxdf));
	wfunc->addNumericParameter( "dist_imm", strus::NumericVariant::asint( g_weightingConfig.distance_imm));
	wfunc->addNumericParameter( "dist_close", strus::NumericVariant::asint( g_weightingConfig.distance_close));
	wfunc->addNumericParameter( "dist_near", strus::NumericVariant::asint( g_weightingConfig.distance_near));
	wfunc->addNumericParameter( "cluster", strus::NumericVariant::asint( g_weightingConfig.minClusterSize));
	wfunc->addNumericParameter( "hotspots", strus::NumericVariant::asint( maxNofRanks));
	wfunc->addNumericParameter( "ffbase", strus::NumericVariant::asdouble( g_weightingConfig.minFfWeight));

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
		strus::QueryResult expected = collection.expectedResult_bm25ff( maxNofRanks, qry, storage.sci.get());
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
	std::cerr << "usage: testWeightingProximity [options] <nofdocs> <nofterms> <nofnodes> <commitsize> <nofqry>" << std::endl;
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

		testWeightingBm25pff( nofDocuments, nofTerms, nofNodes, commitSize, nofQueryies, selectDocid, selectQuery);
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


