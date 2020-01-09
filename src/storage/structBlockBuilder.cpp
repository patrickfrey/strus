/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "structBlockBuilder.hpp"
#include "structBlock.hpp"
#include "structBlockLink.hpp"
#include "strus/base/string_format.hpp"
#include "private/internationalization.hpp"
#include <cstring>
#include <limits>
#include <algorithm>
#include <list>

#undef STRUS_LOWLEVEL_DEBUG
using namespace strus;

StructBlockBuilder::StructBlockBuilder( strus::Index docno_, const std::vector<StructBlockDeclaration>& declarations_)
	:m_map(),m_docno(docno_),m_indexCount(0)
{
	std::vector<StructBlockDeclaration>::const_iterator di = declarations_.begin(), de = declarations_.end();
	for (; di != de; ++di)
	{
		(void)append( di->structno, di->src, di->sink);
	}
}

StructBlockBuilder::StructBlockBuilder( const StructBlock& blk)
	:m_map(),m_docno(blk.id()),m_indexCount(0)
{
	std::vector<StructBlockDeclaration> declist = blk.declarations();
	std::vector<StructBlockDeclaration>::const_iterator
		di = declist.begin(), de = declist.end();
	for (; di != de; ++di)
	{
		(void)append( di->structno, di->src, di->sink);
	}
}


bool StructBlockBuilder::append( strus::Index structno, const strus::IndexRange& src, const strus::IndexRange& sink)
{
	if (!src.defined() || !sink.defined())
	{
		throw strus::runtime_error(_TXT("relation with undefined source or sink added to structure block"));
	}
	if (0>=structno)
	{
		throw strus::runtime_error(_TXT("relation with bad structure id added to structure block"));
	}
	if (structno > StructBlock::MaxNofStructNo)
	{
		throw strus::runtime_error(_TXT("structure number (%d), out of range, only %d different structure types allowed"), (int)structno, (int)StructBlock::MaxNofStructNo);
	}
	int structidx = m_map.findStructureHeader( src, structno);
	if (structidx > 0)
	{
		strus::IndexRange pred_range( 0, sink.start());
		IndexRangeLinkMap::const_iterator pi = m_map.first( pred_range), pe = m_map.end();
		for (; pi != pe && pi->range.end() == sink.start(); ++pi)
		{
			if (pi->link.structno == structno && pi->link.idx == structidx && pi->link.head == false)
			{
				strus::IndexRange sink_expanded( pi->range.start(), sink.end());
				StructBlockLink lnk( structno, false/*head*/, structidx);
				m_map.erase( pi);
				return m_map.append( sink_expanded, lnk);
			}
		}
		return m_map.append( sink, StructBlockLink( structno, false/*head*/, structidx));
	}
	else
	{
		structidx = ++m_indexCount;
		if (m_indexCount >= StructBlock::MaxNofStructIdx)
		{
			throw strus::runtime_error(_TXT("number of structures (%d), out of range, only %d different structure types allowed"), (int)m_indexCount, (int)StructBlock::MaxNofStructIdx);
		}
		bool rt = false;
		rt |= m_map.append( src, StructBlockLink( structno, true/*head*/, structidx));
		rt |= m_map.append( sink, StructBlockLink( structno, false/*head*/, structidx));
		return rt;
	}
}

std::vector<StructBlockDeclaration> StructBlockBuilder::declarations() const
{
	std::vector<StructBlockDeclaration> rt;
	IndexRangeLinkMap::inv_iterator si = m_map.inv_begin(), se = m_map.inv_end();
	for (; si != se && si->link.head; ++si)
	{
		StructBlockLink memberLink( si->link.structno, false/*head*/, si->link.idx);
		IndexRangeLinkMap::inv_iterator mi = m_map.inv_first( memberLink);
		for (; mi->link == memberLink; ++mi)
		{
			rt.push_back( StructBlockDeclaration( si->link.structno, si->range/*src*/, mi->range/*sink*/));
		}
	}
	return rt;
}

void StructBlockBuilder::check() const
{
	std::size_t nofDeclarations = declarations().size();
	if (size() != nofDeclarations)
	{
		throw strus::runtime_error(_TXT("number of structures do not match (%d != %d)"), (int)size(), (int)nofDeclarations);
	}
	IndexRangeLinkMap::const_iterator si = m_map.begin(), se = m_map.end();
	for (; si != se; ++si)
	{
		IndexRangeLinkMap::const_iterator sn = si;
		for (++sn; sn != se; ++sn)
		{
			if (sn->range.overlap( si->range))
			{
				if (sn->link.head && si->link.head
				&&  sn->link.structno == si->link.structno)
				{
					throw strus::runtime_error(_TXT("structure headers are overlapping"));
				}
				if (!sn->link.head && !si->link.head
				&&  sn->link.structno == si->link.structno
				&&  sn->link.idx == si->link.idx)
				{
					throw strus::runtime_error(_TXT("structure members are overlapping"));
				}
			}
		}
	}
}

typedef StructBlockBuilder::FieldCover FieldCover;
typedef StructBlockBuilder::IndexRangeLinkPair MapElement;

class FieldAccessFunctor_MapElement
{
public:
	FieldAccessFunctor_MapElement(){}
	const strus::IndexRange& operator()( const MapElement& obj) const
	{
		return obj.range;
	}
};

class FieldAccessFunctor_IndexRange
{
public:
	FieldAccessFunctor_IndexRange(){}
	const strus::IndexRange& operator()( const strus::IndexRange& obj) const
	{
		return obj;
	}
};

struct IndexRangeTreeNode
{
	strus::IndexRange range;
	std::list<IndexRangeTreeNode> chld;

	IndexRangeTreeNode( const strus::IndexRange& range_)
		:range(range_),chld(){}
	IndexRangeTreeNode( const IndexRangeTreeNode& o)
		:range(o.range),chld(o.chld){}

	void add( const IndexRangeTreeNode& nd)
		{chld.push_back(nd);}
	void swap( IndexRangeTreeNode& nd)
	{
		chld.swap( nd.chld);
		std::swap( range, nd.range);
	}
};

struct IndexRangeLevelAssingment
{
	strus::IndexRange range;
	int level;

	IndexRangeLevelAssingment( const strus::IndexRange& range_, int level_)
		:range(range_),level(level_){}
	IndexRangeLevelAssingment( const IndexRangeLevelAssingment& o)
		:range(o.range),level(o.level){}

	bool operator < (const IndexRangeLevelAssingment& o) const
	{
		return (level == o.level) ? (range < o.range) : (level < o.level);
	}
};

static bool addIndexRangeTreeNode( IndexRangeTreeNode& tree, const strus::IndexRange& elem)
{
	if (elem.overlap( tree.range))
	{
		if (elem.cover( tree.range))
		{
			return false;
		}
		else if (tree.range.cover( elem))
		{
			std::list<IndexRangeTreeNode>::iterator ci = tree.chld.begin(), ce = tree.chld.end();
			for (; ci != ce; ++ci)
			{
				if (elem.cover( ci->range))
				{
					if (elem == ci->range) return true;

					IndexRangeTreeNode covernode( elem);
					covernode.add( *ci);
					ci = tree.chld.erase( ci);
					ce = tree.chld.end();
					while (ci != ce)
					{
						if (covernode.range.cover( ci->range))
						{
							covernode.add( *ci);
							ci = tree.chld.erase( ci);
							ce = tree.chld.end();
						}
						else
						{
							++ci;
						}
					}
					tree.add( covernode);
					return true;
				}
				else if (addIndexRangeTreeNode( *ci, elem))
				{
					return true;
				}
			}
			tree.add( elem);
			return true;
		}
	}
	return false;
}

static std::vector<IndexRangeTreeNode> buildIndexRangeTrees( std::vector<strus::IndexRange>& rest, const std::vector<strus::IndexRange>& llist)
{
	std::vector<IndexRangeTreeNode> rt;
	if (llist.empty()) return rt;

	std::vector<strus::IndexRange>::const_iterator
		li = llist.begin(), le = llist.end();
	for (; li != le; ++li)
	{
		bool added = false;
		std::vector<IndexRangeTreeNode>::iterator
			ri = rt.begin(), re = rt.end();
		while (ri != re && !added)
		{
			if (addIndexRangeTreeNode( *ri, *li))
			{
				added = true;
			}
			else if (*li == ri->range)
			{
				added = true;
			}
			else if (li->cover( ri->range))
			{
				IndexRangeTreeNode covernode( *li);
				covernode.add( *ri);
				ri = rt.erase( ri);
				re = rt.end();
				while (ri != re)
				{
					if (covernode.range.cover( ri->range))
					{
						covernode.add( *ri);
						ri = rt.erase( ri);
						re = rt.end();
					}
					else
					{
						++ri;
					}
				}
				rt.push_back( covernode);
				added = true;
			}
			else if (li->overlap( ri->range))
			{
				rest.push_back( *li);
				added = true;
			}
			else
			{
				++ri;
			}
		}
		if (!added)
		{
			rt.push_back( *li);
		}
	}
	return rt;
}

static void collectIndexRangeTreeLevelAssignments( std::set<IndexRangeLevelAssingment>& result, const IndexRangeTreeNode& node, int depth)
{
	result.insert( IndexRangeLevelAssingment( node.range, depth));
	std::list<IndexRangeTreeNode>::const_iterator ci = node.chld.begin(), ce = node.chld.end();
	for (; ci != ce; ++ci)
	{
		collectIndexRangeTreeLevelAssignments( result, *ci, depth+1);
	}
}

static std::set<IndexRangeLevelAssingment> getIndexRangeTreeLevelAssignments( const std::vector<IndexRangeTreeNode>& trees)
{
	std::set<IndexRangeLevelAssingment> rt;
	std::vector<IndexRangeTreeNode>::const_iterator ni = trees.begin(), ne = trees.end();
	for (; ni != ne; ++ni)
	{
		collectIndexRangeTreeLevelAssignments( rt, *ni, 0);
	}
	return rt;
}

static bool hasOverlappingFields( const FieldCover& field)
{
	if (field.empty()) return true;
	FieldCover::const_iterator fi = field.begin(), fe = field.end(), fn = field.begin();
	++fn;
	for (; fn != fe; ++fi,++fn)
	{
		if (fi->end() > fn->start()) return true;
	}
	return false;
}


static std::vector<FieldCover> getFieldCovers_( const std::vector<strus::IndexRange>& fields, std::vector<strus::IndexRange>& rest)
{
	std::vector<FieldCover> rt;
	if (fields.empty()) return rt;

	std::vector<IndexRangeTreeNode>
		treelist = buildIndexRangeTrees( rest, fields);
	std::set<IndexRangeLevelAssingment>
		rangeLevelAssignments = getIndexRangeTreeLevelAssignments( treelist);

	std::set<IndexRangeLevelAssingment>::const_iterator
		ai = rangeLevelAssignments.begin(), ae = rangeLevelAssignments.end();
	for (; ai != ae; ++ai)
	{
		while (ai->level >= (int)rt.size()) rt.push_back( FieldCover());
		rt[ ai->level].insert( ai->range);
	}
	std::vector<FieldCover>::const_iterator ri = rt.begin(), re = rt.end();
	for (; ri != re; ++ri)
	{
		if (ri->empty())
		{
			throw std::runtime_error(_TXT("logic error: got empty field cover after separation of overlaps to different levels"));
		}
		if (hasOverlappingFields( *ri))
		{
			throw std::runtime_error(_TXT("logic error: got overlapping fields after separation of overlaps to different levels"));
		}
	}
	return rt;
}

std::vector<FieldCover> StructBlockBuilder::getFieldCovers() const
{
	std::vector<strus::IndexRange> rest;
	std::vector<FieldCover> add;
	std::vector<FieldCover> rt = getFieldCovers_( m_map.fields(), rest);
	while (!rest.empty())
	{
		std::vector<strus::IndexRange> fields;
		fields.swap( rest);
		std::vector<FieldCover> restcovers = getFieldCovers_( fields, rest);
		add.insert( add.end(), restcovers.begin(), restcovers.end());
	}
	std::vector<FieldCover>::iterator
		ai = add.begin(), ae = add.end();
	while (ai != ae)
	{
		std::vector<FieldCover>::iterator
			ri = rt.begin(), re = rt.end();
		for (; ri != re; ++ri)
		{
			FieldCover join = *ri;
			join.insert( ai->begin(), ai->end());
			if (!hasOverlappingFields( join))
			{
				*ri = join;
				break;
			}
		}
		if (ri != re)
		{
			add.erase( ai);
			ae = add.end();
		}
		else
		{
			++ai;
		}
	}
	rt.insert( rt.end(), add.begin(), add.end());
	return rt;
}

struct LinkDef
{
	strus::IndexRange range;
	int width;
	int index;

	LinkDef( const strus::IndexRange& range_, int width_, int index_)
		:range(range_),width(width_),index(index_){}
	LinkDef( const LinkDef& o)
		:range(o.range),width(o.width),index(o.index){}
};

struct FieldPackingCandidate
{
	std::vector<LinkDef>::const_iterator itr;
	float weight;
	int count;
	StructBlock::FieldType type;

	explicit FieldPackingCandidate( std::vector<LinkDef>::const_iterator itr_)
		:itr(itr_),weight(0.0),count(0),type(StructBlock::FieldTypeIndex){}
	FieldPackingCandidate( std::vector<LinkDef>::const_iterator itr_, float weight_, StructBlock::FieldType type_, int count_)
		:itr(itr_),weight(weight_),count(count_),type(type_){}
	FieldPackingCandidate( const FieldPackingCandidate& o)
		:itr(o.itr),weight(o.weight),count(o.count),type(o.type){}

	bool operator < (const FieldPackingCandidate& o) const
	{
		return ((weight == o.weight)
			? ((count == o.count)
				? ((itr == o.itr)
					? ((int)type < (int)o.type)
					: (itr > o.itr))
				: (count > o.count))
			:(weight > o.weight));
	}
};

struct FieldPackingDim
{
	int elements;
	int bytes;
	StructBlock::PositionType end;

	FieldPackingDim()
		:elements(0),bytes(0),end(0){}
	FieldPackingDim( int elements_, int unitsize, int width, StructBlock::PositionType end_)
		:elements(elements_)
		,bytes(unitsize + sizeof(StructBlock::StructureField) + (elements_ * width * sizeof(StructBlockLink)) + sizeof(StructBlock::PackedLinkBasePointer))
		,end(end_){}
	FieldPackingDim( const FieldPackingDim& o)
		:elements(o.elements),bytes(o.bytes),end(o.end){}

	float weight() const	{return ((float)elements / (float)bytes);}
	bool defined() const	{return elements&&bytes;}
};

static FieldPackingDim evaluateFieldPackingDim_offset(
		std::vector<LinkDef>::const_iterator si,
		std::vector<LinkDef>::const_iterator se,
		StructBlock::FieldIdx& ofs)
{
	ofs = si->range.end() - si->range.start();
	if (si->range.end() - si->range.start() >= StructBlock::MaxFieldIdx) return FieldPackingDim();
	int width = si->width;
	return FieldPackingDim( 1/*elements*/, 0/*unitsize*/, width, si->range.end());
}

static FieldPackingDim evaluateFieldPackingDim_index(
		std::vector<LinkDef>::const_iterator si,
		std::vector<LinkDef>::const_iterator se,
		StructBlock::PositionType& start)
{
	start = si->range.start();
	int width = si->width;
	return FieldPackingDim( 1/*elements*/, sizeof(StructBlock::PositionType)/*unitsize*/, width, si->range.end());
}

static FieldPackingDim evaluateFieldPackingDim_enum(
		std::vector<LinkDef>::const_iterator si,
		std::vector<LinkDef>::const_iterator se,
		StructBlockFieldEnum& enm)
{
	StructBlock::PositionType end = 0;
	int elements = 0;
	int width = si->width;
	for (; si != se && si->width == width; ++si,++elements)
	{
		if (!enm.append( si->range)) break;
		end = si->range.end();
	}
	return FieldPackingDim( elements, sizeof(StructBlockFieldEnum)/*unitsize*/, width, end);
}

static FieldPackingDim evaluateFieldPackingDim_repeat(
		std::vector<LinkDef>::const_iterator si,
		std::vector<LinkDef>::const_iterator se,
		StructBlockFieldRepeat& rep)
{
	StructBlock::PositionType end = 0;
	int elements = 0;
	int width = si->width;
	if (si + 1 < se)
	{
		std::vector<LinkDef>::const_iterator sn = si+1;
		strus::Index itrend = sn->range.start();
		for (; si != se && si->width == width; ++si,++elements)
		{
			if (!rep.append( itrend, si->range)) break;
			end = si->range.end();
		}
	}
	return FieldPackingDim( elements, sizeof(StructBlockFieldRepeat)/*unitsize*/, width, end);
}

static FieldPackingDim evaluateFieldPackingDim_pkbyte(
		std::vector<LinkDef>::const_iterator si,
		std::vector<LinkDef>::const_iterator se,
		StructBlockFieldPackedByte& pkb)
{
	StructBlock::PositionType end = 0;
	int elements = 0;
	int width = si->width;
	for (; si != se && si->width == width; ++si,++elements)
	{
		if (!pkb.append( si->range)) break;
		end = si->range.end();
	}
	return FieldPackingDim( elements, sizeof(StructBlockFieldPackedByte)/*unitsize*/, width, end);
}

static FieldPackingDim evaluateFieldPackingDim_pkshort(
		std::vector<LinkDef>::const_iterator si,
		std::vector<LinkDef>::const_iterator se,
		StructBlockFieldPackedShort& pks)
{
	StructBlock::PositionType end = 0;
	int elements = 0;
	int width = si->width;
	for (; si != se && si->width == width; ++si,++elements)
	{
		if (!pks.append( si->range)) break;
		end = si->range.end();
	}
	return FieldPackingDim( elements, sizeof(StructBlockFieldPackedShort)/*unitsize*/, width, end);
}

static StructBlock::FieldType getNextPackFieldType(
	std::vector<LinkDef>::const_iterator si,
	std::vector<LinkDef>::const_iterator se)
{
	enum {MaxTryPackDepth=4};
	std::set<FieldPackingCandidate> cdset;
	cdset.insert( FieldPackingCandidate( si));

	while (!cdset.empty())
	{
		std::set<FieldPackingCandidate>::iterator ci = cdset.begin();
		FieldPackingCandidate cd = *ci;
		cdset.erase( ci);

		if (cd.count == MaxTryPackDepth || cd.itr == se) return cd.type;
		if (cd.count >= 1 && cdset.empty()) return cd.type;//... all candidates lead to the same result 
		else
		{{
			StructBlock::FieldIdx ofs;
			FieldPackingDim dim = evaluateFieldPackingDim_offset( cd.itr, se, ofs);
			if (dim.defined())
			{
				StructBlock::FieldType fieldType = cd.count ? cd.type : StructBlock::FieldTypeOffset;
				cdset.insert( FieldPackingCandidate( si+dim.elements, cd.weight + dim.weight(), fieldType, cd.count+1));
			}
		}{
			StructBlock::PositionType start;
			FieldPackingDim dim = evaluateFieldPackingDim_index( cd.itr, se, start);
			if (dim.defined())
			{
				StructBlock::FieldType fieldType = cd.count ? cd.type : StructBlock::FieldTypeIndex;
				cdset.insert( FieldPackingCandidate( si+dim.elements, cd.weight + dim.weight(), fieldType, cd.count+1));
			}
		}{
			StructBlockFieldEnum enm;
			FieldPackingDim dim = evaluateFieldPackingDim_enum( si, se, enm);
			if (dim.defined())
			{
				StructBlock::FieldType fieldType = cd.count ? cd.type : StructBlock::FieldTypeEnum;
				cdset.insert( FieldPackingCandidate( si+dim.elements, cd.weight + dim.weight(), fieldType, cd.count+1));
			}
		}{
			StructBlockFieldRepeat rep;
			FieldPackingDim dim = evaluateFieldPackingDim_repeat( si, se, rep);
			if (dim.defined())
			{
				StructBlock::FieldType fieldType = cd.count ? cd.type : StructBlock::FieldTypeRepeat;
				cdset.insert( FieldPackingCandidate( si+dim.elements, cd.weight + dim.weight(), fieldType, cd.count+1));
			}
		}{
			StructBlockFieldPackedByte pkb;
			FieldPackingDim dim = evaluateFieldPackingDim_pkbyte( si, se, pkb);
			if (dim.defined())
			{
				StructBlock::FieldType fieldType = cd.count ? cd.type : StructBlock::FieldTypePackedByte;
				cdset.insert( FieldPackingCandidate( si+dim.elements, cd.weight + dim.weight(), fieldType, cd.count+1));
			}
		}{
			StructBlockFieldPackedShort pks;
			FieldPackingDim dim = evaluateFieldPackingDim_pkshort( si, se, pks);
			if (dim.defined())
			{
				StructBlock::FieldType fieldType = cd.count ? cd.type : StructBlock::FieldTypePackedShort;
				cdset.insert( FieldPackingCandidate( si+dim.elements, cd.weight + dim.weight(), fieldType, cd.count+1));
			}
		}}
	}
	return StructBlock::FieldTypeIndex;
}

StructBlock StructBlockBuilder::createBlock()
{
	if (docno() <= 0) throw std::runtime_error(_TXT("docno not set when calling create block"));
	std::vector<FieldCover> covers = getFieldCovers();
	if (covers.size() > StructBlock::MaxFieldLevels)
	{
		throw strus::runtime_error(_TXT("number (%d) of levels of overlapping fields exceeds maximum size (%d) allowed"), (int)covers.size(), (int)StructBlock::MaxFieldLevels);
	}
	std::vector<std::vector<StructBlock::StructureField> > fieldar( covers.size());
	std::vector<std::vector<StructBlock::LinkBasePointer> > linkbasear( covers.size());
	std::vector<std::vector<StructBlockLink> > linkar( StructBlock::MaxLinkWidth);
	std::vector<StructBlockFieldEnum> enumar;
	std::vector<StructBlockFieldRepeat> repeatar;
	std::vector<StructBlock::PositionType> startar;
	std::vector<StructBlockFieldPackedByte> pkbytear;
	std::vector<StructBlockFieldPackedShort> pkshortar;

	std::vector<FieldCover>::const_iterator ci = covers.begin(), ce = covers.end();
	int cidx = 0;
	for (; ci != ce; ++ci,++cidx)
	{
		std::vector<LinkDef> linkdefs;
		std::set<strus::IndexRange>::const_iterator
			fi = ci->begin(), fe = ci->end();
		for (; fi != fe; ++fi)
		{
			IndexRangeLinkMap::const_iterator mi = m_map.first( *fi);
			std::set<StructBlockLink> lnkset;
			for (; mi->range == *fi; ++mi)
			{
				lnkset.insert( mi->link);
			}
			int width = lnkset.size();
			if (width > StructBlock::MaxLinkWidth)
			{
				throw strus::runtime_error( _TXT("too many links defined defined per field (%d > maximum %d)"), width, (int)StructBlock::MaxLinkWidth);
			}
			if (width == 0)
			{
				throw std::runtime_error( _TXT("found range not assigned to structure"));
			}
			std::vector<StructBlockLink>& ths_linkar = linkar[ width-1];
			linkdefs.push_back( LinkDef( *fi, width, ths_linkar.size()));
			ths_linkar.insert( ths_linkar.end(), lnkset.begin(), lnkset.end());
		}
		// Compress and store the fields:
		std::vector<LinkDef>::const_iterator li = linkdefs.begin(), le = linkdefs.end();
		while (li != le)
		{
			StructBlock::FieldType fieldType = getNextPackFieldType( li, le);
			FieldPackingDim dim;

			switch (fieldType)
			{
				case StructBlock::FieldTypeOffset:
				{
					StructBlock::FieldIdx ofs;
					dim = evaluateFieldPackingDim_offset( li, le, ofs);
					fieldar[ cidx].push_back( StructBlock::StructureField( dim.end, fieldType, ofs));
					break;
				}
				case StructBlock::FieldTypeIndex:
				{
					StructBlock::PositionType start;
					dim = evaluateFieldPackingDim_index( li, le, start);
					fieldar[ cidx].push_back( StructBlock::StructureField( dim.end, fieldType, startar.size()));
					startar.push_back( start);
					break;
				}
				case StructBlock::FieldTypeEnum:
				{
					StructBlockFieldEnum enm;
					dim = evaluateFieldPackingDim_enum( li, le, enm);
					fieldar[ cidx].push_back( StructBlock::StructureField( dim.end, fieldType, enumar.size()));
					enumar.push_back( enm);
					break;
				}
				case StructBlock::FieldTypeRepeat:
				{
					StructBlockFieldRepeat rep;
					dim = evaluateFieldPackingDim_repeat( li, le, rep);
					fieldar[ cidx].push_back( StructBlock::StructureField( dim.end, fieldType, repeatar.size()));
					repeatar.push_back( rep);
					break;
				}
				case StructBlock::FieldTypePackedByte:
				{
					StructBlockFieldPackedByte pkb;
					dim = evaluateFieldPackingDim_pkbyte( li, le, pkb);
					fieldar[ cidx].push_back( StructBlock::StructureField( dim.end, fieldType, pkbytear.size()));
					pkbytear.push_back( pkb);
					break;
				}
				case StructBlock::FieldTypePackedShort:
				{
					StructBlockFieldPackedShort pks;
					dim = evaluateFieldPackingDim_pkshort( li, le, pks);
					fieldar[ cidx].push_back( StructBlock::StructureField( dim.end, fieldType, pkshortar.size()));
					pkshortar.push_back( pks);
					break;
				}
			}
			linkbasear[ cidx].push_back( StructBlock::LinkBasePointer( li->index, li->width));
			li += dim.elements;
		}
	}
	StructBlock rt( docno(), fieldar, linkbasear, linkar, enumar, repeatar, startar, pkbytear, pkshortar);
#ifdef STRUS_LOWLEVEL_DEBUG
	std::vector<StructBlockDeclaration> declist_build = this->declarations();
	std::vector<StructBlockDeclaration> declist_block = rt.declarations();
	std::vector<StructBlockDeclaration>::const_iterator ui = declist_build.begin(), ue = declist_build.end();
	std::vector<StructBlockDeclaration>::const_iterator li = declist_block.begin(), le = declist_block.end();
	for (; li != le && ui != ue; ++li,++ui)
	{
		if (*ui != *li) throw std::runtime_error(_TXT("structures built are currupt"));
	}
	if (li != le || ui != ue) throw std::runtime_error(_TXT("structures built are currupt"));
#endif
	return rt;
}

void StructBlockBuilder::print( std::ostream& out) const
{
	out << "docno " << m_docno << ":" << std::endl;
	IndexRangeLinkMap::const_iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		mi->link.print( out);
		out << " [" << mi->range.start() << "," << mi->range.end() << "]" << std::endl;
	}
}



