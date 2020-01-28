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
#include "strus/lib/fieldtrees.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/base/localErrorBuffer.hpp"
#include "private/internationalization.hpp"
#include <cstring>
#include <limits>
#include <algorithm>

#ifdef NDEBUG
#undef STRUS_LOWLEVEL_DEBUG
#else
#define STRUS_LOWLEVEL_DEBUG
#endif
using namespace strus;

StructBlockBuilder::StructBlockBuilder( const std::string& docid_, strus::Index docno_, const std::vector<StructBlockDeclaration>& declarations_, ErrorBufferInterface* errorhnd_)
	:m_map(),m_headerar(),m_docid(docid_),m_docno(docno_),m_indexCount(0),m_errorhnd(errorhnd_)
{
	std::vector<StructBlockDeclaration>::const_iterator di = declarations_.begin(), de = declarations_.end();
	for (; di != de; ++di)
	{
		(void)append( di->structno, di->src, di->sink);
	}
}

StructBlockBuilder::StructBlockBuilder( const StructBlock& blk, const std::string& docid_, ErrorBufferInterface* errorhnd_)
	:m_map(),m_headerar(),m_docid(docid_),m_docno(blk.id()),m_indexCount(0),m_errorhnd(errorhnd_)
{
	m_headerar.reserve( blk.headerar().size);
	int hi=0, he=blk.headerar().size;
	for (; hi != he; ++hi)
	{
		m_headerar.push_back( blk.headerar()[ hi]);
	}
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
	bool rt = false;
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
		rt = m_map.append( sink, StructBlockLink( structno, false/*head*/, structidx));
	}
	else
	{
		structidx = ++m_indexCount;
		if (m_indexCount >= StructBlock::MaxNofStructIdx)
		{
			throw strus::runtime_error(_TXT("number of structures (%d), out of range, only %d different structure types allowed"), (int)m_indexCount, (int)StructBlock::MaxNofStructIdx);
		}
		rt |= m_map.append( src, StructBlockLink( structno, true/*head*/, structidx));
		rt |= m_map.append( sink, StructBlockLink( structno, false/*head*/, structidx));
		m_headerar.push_back( src.start());
		if ((int)m_headerar.size() != m_indexCount)
		{
			throw strus::runtime_error(_TXT("data corruption, structure index not key index for header start"));
		}
	}
	return rt;
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

static void collectFieldTreeLevelAssignments( std::set<IndexRangeLevelAssingment>& result, const FieldTree& node, int depth)
{
	result.insert( IndexRangeLevelAssingment( node.range, depth));
	std::list<FieldTree>::const_iterator ci = node.chld.begin(), ce = node.chld.end();
	for (; ci != ce; ++ci)
	{
		collectFieldTreeLevelAssignments( result, *ci, depth+1);
	}
}

static std::set<IndexRangeLevelAssingment> getFieldTreeLevelAssignments( const std::vector<FieldTree>& trees)
{
	std::set<IndexRangeLevelAssingment> rt;
	std::vector<FieldTree>::const_iterator ni = trees.begin(), ne = trees.end();
	for (; ni != ne; ++ni)
	{
		collectFieldTreeLevelAssignments( rt, *ni, 0);
	}
	return rt;
}

static std::pair<strus::IndexRange,strus::IndexRange> overlappingFields( const FieldCover& field)
{
	FieldCover::const_iterator fi = field.begin(), fe = field.end(), fn = field.begin();
	++fn;
	for (; fn != fe; ++fi,++fn)
	{
		if (fi->end() > fn->start()) return std::pair<strus::IndexRange,strus::IndexRange>(*fi,*fn);
	}
	return std::pair<strus::IndexRange,strus::IndexRange>(strus::IndexRange(),strus::IndexRange());
}

static void checkCoverValidity( const FieldCover& cover)
{
	if (cover.empty())
	{
		throw std::runtime_error(_TXT("logic error: got empty field after separation of overlaps to different levels"));
	}
	std::pair<strus::IndexRange,strus::IndexRange> ovl = overlappingFields( cover);
	if (ovl.first.defined())
	{
		throw strus::runtime_error(_TXT("logic error: got overlapping fields [%d,%d] and [%d,%d] after separation of overlaps to different levels"), 
				(int)ovl.first.start(),(int)ovl.first.end(),
				(int)ovl.second.start(),(int)ovl.second.end());
	}
}

static std::vector<FieldCover> getFieldCovers_( const std::vector<strus::IndexRange>& fields, std::vector<strus::IndexRange>& rest)
{
	LocalErrorBuffer errorbuf;
	std::vector<FieldCover> rt;
	if (fields.empty()) return rt;

	std::vector<FieldTree> treelist = buildFieldTrees( rest, fields, &errorbuf);
	if (treelist.empty()) throw std::runtime_error( errorbuf.fetchError());

	std::set<IndexRangeLevelAssingment> rangeLevelAssignments = getFieldTreeLevelAssignments( treelist);

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
		checkCoverValidity( *ri);
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
			if (!overlappingFields( join).first.defined())
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
		,bytes(unitsize + sizeof(StructBlock::StructureField) + (elements_ * width * sizeof(StructBlockLink)) + sizeof(StructBlock::LinkBasePointer))
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
	if (si + 1 >= se) return FieldPackingDim();

	std::vector<LinkDef>::const_iterator sn = si+1;
	if (sn->width != si->width) return FieldPackingDim();
	strus::Index nextStart;
	if (!rep.appendFirstPair( nextStart, si->range, sn->range)) return FieldPackingDim();
	end = sn->range.end();
	si += 2;

	for (; si != se && si->width == width; ++si,++elements)
	{
		if (!rep.appendNext( nextStart, si->range)) break;
		end = si->range.end();
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

static std::string getFieldDependencyDescription(
		const StructBlockBuilder::IndexRangeLinkMap& map, const std::set<StructBlockLink>& lnkset)
{
	std::string rt;
	std::set<StructBlockLink>::const_iterator li = lnkset.begin(), le = lnkset.end();
	for (; li != le; ++li)
	{
		StructBlockBuilder::IndexRangeLinkMap::inv_iterator vi = map.inv_first( *li);
		if (vi == map.inv_end()) throw std::runtime_error(_TXT("logic error: structure link not found"));
		if (!rt.empty()) rt.push_back(',');
		rt.append( strus::string_format("%c(%d|%d)[%d,%d]",
				li->head ? 'H':'C', (int)li->structno, (int)li->idx,
				(int)vi->range.start(), (int)vi->range.end()));
	}
	return rt;
}

static std::set<StructBlockKey> getOverflowStructureList( const StructBlockBuilder::IndexRangeLinkMap& map)
{
	std::set<StructBlockKey> rt;
	StructBlockBuilder::IndexRangeLinkMap::const_iterator
		mi = map.begin(), me = map.end();
	while (mi != me)
	{
		IndexRange field = mi->range;
		std::set<StructBlockLink> lnkset;
		for (; mi != me && mi->range == field; ++mi)
		{
			lnkset.insert( mi->link);
			if (lnkset.size()-1 > StructBlock::MaxLinkWidth)
			{
				lnkset.erase( mi->link);
				rt.insert( StructBlockKey( mi->link.structno, mi->link.idx));
			}
		}
	}
	return rt;
}

void StructBlockBuilder::eliminateStructuresBeyondCapacity()
{
	std::set<StructBlockKey> stuset = getOverflowStructureList( m_map);
	int nofStructures = stuset.size();
	if (nofStructures)
	{
		std::vector<StructBlockDeclaration> deletes;
		std::set<StructBlockKey>::const_iterator si = stuset.begin(), se = stuset.end();
		for (; si != se; ++si)
		{
			m_map.removeStructure( si->structno, si->idx, deletes);
			m_headerar[ si->idx-1] = 0;
		}
		m_errorhnd->info( _TXT("warning: had to ignore %d structure relations in document '%s' due to complexity (more than %d links for a field)"), (int)deletes.size(), m_docid.c_str(), (int)(StructBlock::MaxLinkWidth+1));
	}
}

StructBlock StructBlockBuilder::createBlock()
{
	eliminateStructuresBeyondCapacity();

	std::vector<FieldCover> covers = getFieldCovers();
	if (covers.size() > StructBlock::NofFieldLevels)
	{
		throw strus::runtime_error(_TXT("number (%d) of levels of overlapping fields exceeds maximum size (%d) allowed"), (int)covers.size(), (int)StructBlock::NofFieldLevels);
	}
	std::vector<std::vector<StructBlock::StructureField> > fieldar( covers.size());
	std::vector<std::vector<StructBlock::LinkBasePointer> > linkbasear( covers.size());
	std::vector<std::vector<StructBlockLink> > linkar( StructBlock::MaxLinkWidth+1);
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
		std::set<strus::IndexRange>::const_iterator fi = ci->begin(), fe = ci->end();
		for (; fi != fe; ++fi)
		{
			IndexRangeLinkMap::const_iterator mi = m_map.first( *fi);
			std::set<StructBlockLink> lnkset;
			for (; mi->range == *fi; ++mi)
			{
				lnkset.insert( mi->link);
			}
			if (lnkset.empty())
			{
				continue;
			}
			int width = lnkset.size()-1;
			if (width > StructBlock::MaxLinkWidth)
			{
				std::string depdescr = getFieldDependencyDescription( m_map, lnkset);
				throw strus::runtime_error( _TXT("too many links (%d > maximum %d) defined defined per field [%d,%d] => {%s}"),
						width+1, (int)StructBlock::MaxLinkWidth+1,
						(int)fi->start(), (int)fi->end(), depdescr.c_str());
			}
			std::vector<StructBlockLink>& ths_linkar = linkar[ width];
			linkdefs.push_back( LinkDef( *fi, width, ths_linkar.size() / (width+1)));
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
			if (li->index >= StructBlock::MaxLinkBaseIdx)
			{
				strus::runtime_error(_TXT("too many structures defined, link base index (%d) out of range"), li->index);
			}
			if (li->width >= StructBlock::MaxLinkWidth)
			{
				strus::runtime_error(_TXT("logic error: structure number of links (%d) out of range"), li->width);
			}
			linkbasear[ cidx].push_back( StructBlock::LinkBasePointer( li->index, li->width));
			li += dim.elements;
		}
	}
	std::vector<StructBlock::PositionType> headerar( m_headerar);
	{
		std::size_t hi = headerar.size();
		while (hi > 0 && headerar[ hi-1] == 0) --hi;
		headerar.resize( hi);
	}
	StructBlock rt( docno(), fieldar, linkbasear, linkar, enumar, repeatar, startar, pkbytear, pkshortar, headerar);
#ifdef STRUS_LOWLEVEL_DEBUG
	int fi = 0,fe = rt.fieldarsize();
	for (; fi != fe; ++fi)
	{
		const StructBlock::StructureFieldArray& blk_fieldar = rt.fieldar( fi);
		if (blk_fieldar.size != fieldar[fi].size()) throw strus::runtime_error(_TXT("logic error: corrupt block data structure built (line %d)"), (int)__LINE__);
		if (0!=std::memcmp( blk_fieldar.ar, fieldar[fi].data(), blk_fieldar.size * sizeof( StructBlock::StructureField))) throw strus::runtime_error(_TXT("logic error: corrupt block data structure built (line %d)"), (int)__LINE__);
		const StructBlock::LinkBasePointerArray& blk_linkbasear = rt.linkbasear( fi);
		if (blk_linkbasear.size != linkbasear[fi].size()) throw strus::runtime_error(_TXT("logic error: corrupt block data structure built (line %d)"), (int)__LINE__);
		if (0!=std::memcmp( blk_linkbasear.ar, linkbasear[fi].data(), blk_linkbasear.size * sizeof( StructBlock::LinkBasePointer))) throw strus::runtime_error(_TXT("logic error: corrupt block data structure built (line %d)"), (int)__LINE__);
	}
	int wi = 0, we = StructBlock::MaxLinkWidth;
	for (; wi != we; ++wi)
	{
		const StructBlock::LinkArray& blk_linkar = rt.linkar( wi);
		if ((int)linkar.size() <= wi)
		{
			if (blk_linkar.size != 0) throw strus::runtime_error(_TXT("logic error: corrupt block data structure built (line %d)"), (int)__LINE__);
		}
		else
		{
			if (blk_linkar.size != linkar[wi].size()) throw strus::runtime_error(_TXT("logic error: corrupt block data structure built (line %d)"), (int)__LINE__);
			if (0!=std::memcmp( blk_linkar.ar, linkar[wi].data(), blk_linkar.size * sizeof(StructBlockLink))) throw strus::runtime_error(_TXT("logic error: corrupt block data structure built (line %d)"), (int)__LINE__);
		}
	}
	const StructBlockFieldEnum* blk_enumar = rt.enumar();
	if (0!=std::memcmp( blk_enumar, enumar.data(), enumar.size() * sizeof( blk_enumar[0]))) throw strus::runtime_error(_TXT("logic error: corrupt block data structure built (line %d)"), (int)__LINE__);

	const StructBlockFieldRepeat* blk_repeatar = rt.repeatar();
	if (0!=std::memcmp( blk_repeatar, repeatar.data(), repeatar.size() * sizeof( blk_repeatar[0]))) throw strus::runtime_error(_TXT("logic error: corrupt block data structure built (line %d)"), (int)__LINE__);

	const StructBlock::PositionType* blk_startar = rt.startar();
	if (0!=std::memcmp( blk_startar, startar.data(), startar.size() * sizeof( blk_startar[0]))) throw strus::runtime_error(_TXT("logic error: corrupt block data structure built (line %d)"), (int)__LINE__);

	const StructBlockFieldPackedByte* blk_pkbytear = rt.pkbytear();
	if (0!=std::memcmp( blk_pkbytear, pkbytear.data(), pkbytear.size() * sizeof( blk_pkbytear[0]))) throw strus::runtime_error(_TXT("logic error: corrupt block data structure built (line %d)"), (int)__LINE__);

	const StructBlockFieldPackedShort* blk_pkshortar = rt.pkshortar();
	if (0!=std::memcmp( blk_pkshortar, pkshortar.data(), pkshortar.size() * sizeof( blk_pkshortar[0]))) throw strus::runtime_error(_TXT("logic error: corrupt block data structure built (line %d)"), (int)__LINE__);

	const StructBlock::HeaderStartArray& blk_headerar = rt.headerar();
	if (blk_headerar.size != headerar.size()
	||  0!=std::memcmp( blk_headerar.ar, headerar.data(), headerar.size() * sizeof( blk_headerar[0]))) throw strus::runtime_error(_TXT("logic error: corrupt block data structure built (line %d)"), (int)__LINE__);

	std::vector<StructBlockDeclaration> declist_build = this->declarations();
	std::sort( declist_build.begin(), declist_build.end());

	std::vector<StructBlockDeclaration> declist_block = rt.declarations();
	std::sort( declist_block.begin(), declist_block.end());

	{
		std::vector<StructBlockDeclaration>::const_iterator ui = declist_build.begin(), ue = declist_build.end();
		std::vector<StructBlockDeclaration>::const_iterator li = declist_block.begin(), le = declist_block.end();
		for (int uidx=0; li != le && ui != ue; ++li,++ui,++uidx)
		{
			if (*ui != *li)
			{
				throw std::runtime_error(_TXT("structures built differ from expected"));
			}
		}
		if (li != le || ui != ue)
		{
			throw std::runtime_error(_TXT("structures built differ from expected"));
		}
	}{
		std::vector<StructBlock::PositionType>::const_iterator
			hi = m_headerar.begin(), he = m_headerar.end();
		int structidx = 1;
		for (; hi != he; ++hi,++structidx)
		{
			if (*hi == 0) continue;/*structure was deleted*/

			int li=0, le=rt.fieldarsize();
			for (; li != le; ++li)
			{
				StructBlock::FieldScanner scan = rt.fieldscanner( li);
				strus::IndexRange field = scan.skip( *hi);
				if (field.start() == (strus::Index)*hi)
				{
					const StructureLinkArray& lar = scan.links();
					int xi = 0, xe = lar.nofLinks();
					for (; xi != xe; ++xi)
					{
						if (lar[ xi].header() && lar[ xi].index() == structidx) break;
					}
					if (xi != xe) break;
				}
			}
			if (li == le)
			{
				throw std::runtime_error(_TXT("corrupt structure block: header structure lost"));
			}
		}
	}
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

bool StructBlockBuilder::IndexRangeLinkMap::append( const strus::IndexRange& range, const StructBlockLink& link)
{
	bool rt = false;
	rt |= map.insert( IndexRangeLinkPair( range, link)).second;
	rt |= invmap.insert( LinkIndexRangePair( link, range)).second;
	return rt;
}

void StructBlockBuilder::IndexRangeLinkMap::erase( Map::const_iterator mi)
{
	map.erase( IndexRangeLinkPair( mi->range, mi->link));
	invmap.erase( LinkIndexRangePair( mi->link, mi->range));
}

void StructBlockBuilder::IndexRangeLinkMap::removeStructure( strus::Index structno, unsigned int idx, std::vector<StructBlockDeclaration>& deletes)
{
	std::vector<LinkIndexRangePair> removelist;
	StructBlockLink hl( structno, true/*head*/, idx);
	StructBlockLink cl( structno, false/*head*/, idx);
	InvMap::iterator ii = invmap.lower_bound( LinkIndexRangePair( hl, strus::IndexRange()));
	strus::IndexRange src;
	for (;ii != invmap.end() && ii->link == hl; ++ii)
	{
		if (src.defined()) throw std::runtime_error(_TXT("corrupt index: structure header defined twice for same structure"));
		src = ii->range;
		removelist.push_back( LinkIndexRangePair( ii->link, ii->range));
	}
	ii = invmap.lower_bound( LinkIndexRangePair( cl, strus::IndexRange()));
	for (;ii != invmap.end() && ii->link == cl; ++ii)
	{
		deletes.push_back( StructBlockDeclaration( structno, src, ii->range));
		removelist.push_back( LinkIndexRangePair( ii->link, ii->range));
	}
	std::vector<LinkIndexRangePair>::const_iterator
		ri = removelist.begin(), re = removelist.end();
	for (; ri != re; ++ri)
	{
		map.erase( IndexRangeLinkPair( ri->range, ri->link));
		invmap.erase( LinkIndexRangePair( ri->link, ri->range));
	}
}

int StructBlockBuilder::IndexRangeLinkMap::findStructureHeader( const strus::IndexRange& range, strus::Index structno)
{
	Map::const_iterator ri = first( range), re = end();
	for (; ri != re && ri->range == range; ++ri)
	{
		if (ri->link.head && ri->link.structno == structno) return ri->link.idx;
	}
	return -1;
}

int StructBlockBuilder::IndexRangeLinkMap::maxIndex() const
{
	int maxidx = 0;
	InvMap::const_iterator ri = inv_begin(), re = inv_end();
	for (; ri != re && ri->link.head; ++ri)
	{
		if (ri->link.idx > maxidx) maxidx = ri->link.idx;
	}
	return maxidx;
}

strus::IndexRange StructBlockBuilder::IndexRangeLinkMap::lastSource() const
{
	if (map.empty()) return strus::IndexRange();
	Map::reverse_iterator ri = map.rend(), re = map.rbegin();
	for (; ri != re && !ri->link.head; ++ri){}
	return (ri != re) ? ri->range : strus::IndexRange();
}

