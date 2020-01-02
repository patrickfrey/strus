/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "structBlockBuilder.hpp"
#include "structBlock.hpp"
#include "private/internationalization.hpp"
#include <cstring>
#include <limits>
#include <algorithm>

using namespace strus;

StructBlockBuilder::StructBlockBuilder( strus::Index docno_, const std::vector<StructBlockDeclaration>& declarations_)
	:m_map(),m_docno(docno_),m_indexCount(0),m_lastSource()
{
	std::vector<StructBlockDeclaration>::const_iterator di = declarations_.begin(), de = declarations_.end();
	for (; di != de; ++di)
	{
		(void)append( di->structno, di->src, di->sink);
	}
}

StructBlockBuilder::StructBlockBuilder( const StructBlock& blk)
{
	typedef std::map<StructBlockKey,std::vector<StructBlockDeclaration> > DeclMap;
	DeclMap declmap;

	int fi = 0, fe = blk.fieldarsize();
	for (; fi != fe; ++fi)
	{
		StructBlock::FieldScanner scanner = blk.fieldscanner( fi);
		int idx = 0;
		strus::IndexRange field = scanner.next();
		for (; field.defined(); field = scanner.next(),++idx)
		{
			const StructBlockLink* links = scanner.links();
			int li = 0, le = scanner.noflinks();
			for (; li != le; ++li)
			{
				StructBlockKey key( links[ li].structno, links[ li].idx);
				DeclMap::iterator di = declmap.find( key);
				if (di == declmap.end())
				{
					di = declmap.insert( DeclMap::value_type( key, std::vector<StructBlockDeclaration>())).first;
				}
				if (links[ li].head)
				{
					if (di->second.empty())
					{
						di->second.push_back( StructBlockDeclaration( key.structno, field, strus::IndexRange()));
					}
					else if (di->second.back().src.defined())
					{
						if (di->second.back().src != field) throw std::runtime_error(_TXT("currupt index: overlapping duplicate structure key"));
					}
					else
					{
						std::vector<StructBlockDeclaration>::iterator
							ei = di->second.begin(), ee = di->second.end();
						for (; ei != ee; ++ei)
						{
							ei->src = field;
						}
					}
				}
				else
				{
					if (di->second.empty())
					{
						di->second.push_back( StructBlockDeclaration( key.structno, strus::IndexRange(), field));
					}
					else
					{
						di->second.push_back( StructBlockDeclaration( key.structno, di->second.back().src, field));
					}
				}
			}
		}
	}
	DeclMap::iterator mi = declmap.begin(), me = declmap.end();
	for (; mi != me; ++mi)
	{
		std::vector<StructBlockDeclaration>& declist = mi->second;
		std::sort( declist.begin(), declist.end());

		std::vector<StructBlockDeclaration>::const_iterator
			di = declist.begin(), de = declist.end();
		for (; di != de; ++di)
		{
			(void)append( di->structno, di->src, di->sink);
		}
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
	if (src != m_lastSource)
	{
		++m_indexCount;
		m_lastSource = src;
		if (m_map.headerExists( src, structno))
		{
			throw strus::runtime_error(_TXT("members of relation not appended to structure block after header"));
		}
	}
	else
	{
		strus::IndexRange pred_range( 0, sink.start());
		IndexRangeLinkMap::const_iterator pi = m_map.first( pred_range), pe = m_map.end();
		for (; pi != pe && pi->range.end() == pred_range.end(); ++pi)
		{
			if (pi->link.structno == structno && pi->link.idx == m_indexCount && pi->link.head == false)
			{
				strus::IndexRange sink_expanded( pi->range.start(), sink.end());
				m_map.erase( sink, StructBlockLink( false, structno, m_indexCount));
				(void)m_map.append( sink_expanded, StructBlockLink( false, structno, m_indexCount));
			}
		}
	}
	bool rt = false;
	rt |= m_map.append( src, StructBlockLink( true, structno, m_indexCount));
	rt |= m_map.append( sink, StructBlockLink( false, structno, m_indexCount));
	return rt;
}

std::vector<StructBlockDeclaration> StructBlockBuilder::declarations() const
{
	std::vector<StructBlockDeclaration> rt;
	IndexRangeLinkMap::inv_iterator si = m_map.inv_begin(), se = m_map.inv_end();
	for (; si != se && si->link.head; ++si)
	{
		StructBlockLink memberLink( false, si->link.structno, si->link.idx);
		IndexRangeLinkMap::inv_iterator mi = m_map.inv_first( memberLink);
		for (; si->link == memberLink; ++si)
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

template <class FieldList, class FieldAccessFunctor>
static void separateFieldCover( FieldCover& rest, FieldCover& cur, const FieldList& flist)
{
	FieldAccessFunctor getter;
	typename FieldList::const_iterator fi = flist.begin(), fe = flist.end();

	while (fi != fe)
	{
		std::vector<strus::IndexRange> candidates;
		strus::Index cend = getter( *fi).end();
		for (; fi != fe && getter( *fi).end() == cend; ++fi)
		{
			candidates.push_back( getter( *fi));
		}
		strus::IndexRange lastCandidate = candidates.back();
		candidates.pop_back();
	
		std::vector<strus::IndexRange>::const_iterator ci = candidates.begin(), ce = candidates.end();
		for (; ci != ce; ++ci)
		{
			rest.insert( *ci);
		}
		if (cur.empty() || cur.rbegin()->end() <= lastCandidate.start())
		{
			cur.insert( lastCandidate);
		}
		else
		{
			rest.insert( lastCandidate);
		}
	}
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

static bool joinAdjacentFields( FieldCover& upfield, FieldCover& downfield)
{
	bool changed = false;
	FieldCover upfield_new;
	FieldCover downfield_new;

	if (upfield.empty())
	{
		upfield.swap( downfield);
		return true;
	}
	if (downfield.empty())
	{
		return false;
	}
	FieldCover::const_iterator ui = upfield.begin(), ue = upfield.end();
	FieldCover::const_iterator di = downfield.begin(), de = downfield.end();

	while (ui != ue && di != de)
	{
		if (di->end() <= ui->start())
		{
			changed = true;
			upfield_new.insert( *di); ++di;
		}
		else if (di->start() >= ui->end())
		{
			upfield_new.insert( *ui); ++ui;
			if (ui == upfield.end() || ui->start() >= di->end())
			{
				changed = true;
				upfield_new.insert( *di); ++di;
			}
			else
			{
				downfield_new.insert( *di); ++di;
			}
		}
		else
		{
			//... U & D are overlapping
			downfield_new.insert( *di); ++di;
		}
	}
	strus::Index uend = upfield.rend()->end();
	for (; di != de; ++di)
	{
		if (di->start() > uend)
		{
			changed = true;
			upfield_new.insert( *di);
		}
		else
		{
			downfield_new.insert( *di);
		}
	}
	for (; ui != ue; ++ui)
	{
		changed = true;
		upfield_new.insert( *ui);
	}
	if (upfield_new.size() + downfield_new.size() != upfield.size() + downfield.size())
	{
		throw std::runtime_error(_TXT("logic error: lost elements when rearranging field covers"));
	}
	if (hasOverlappingFields( upfield_new) || hasOverlappingFields( downfield_new))
	{
		throw std::runtime_error(_TXT("logic error: got overlaps when rearranging field covers"));
	}
	upfield.swap( upfield_new);
	downfield.swap( downfield_new);
	return changed;
}

std::vector<FieldCover> StructBlockBuilder::getFieldCovers() const
{
	std::vector<FieldCover> rt;
	FieldCover cur;
	FieldCover rest;
	FieldCover plist;

	if (m_map.empty()) return rt;
	separateFieldCover<IndexRangeLinkMap,FieldAccessFunctor_MapElement>( rest, cur, m_map);

	rt.push_back( FieldCover());
	rt.back().swap( cur);

	while (!rest.empty())
	{
		std::size_t restsize = rest.size();
		plist.swap( rest);

		separateFieldCover<FieldCover,FieldAccessFunctor_IndexRange>( rest, cur, plist);
		plist.clear();

		rt.push_back( FieldCover());
		rt.back().swap( cur);
		if (restsize == rest.size())
		{
			throw std::runtime_error(_TXT("overlapping structures of same type detected building structure block"));
		}
	}
	bool changed = false;
	do
	{
		std::size_t ri = rt.size()-1;
		for (; ri > 0; --ri)
		{
			changed |= joinAdjacentFields( rt[ ri], rt[ ri-1]);
		}
		ri = rt.size()-1;
		while (ri >= 0)
		{
			if (rt[ ri].empty())
			{
				changed = true;
				rt.erase( rt.begin() + ri);
			}
			else
			{
				--ri;
			}
		}
	} while (changed);

	std::reverse( rt.begin(), rt.end());
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
		,bytes(unitsize + sizeof(StructBlock::StructureField) + (width * sizeof(PackedStructBlockLink)) + sizeof(StructBlock::PackedLinkBasePointer))
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
	if (covers.size() >= StructBlock::MaxFieldLevels) throw strus::runtime_error(_TXT("number (%d) of levels of overlapping fields exceeds maximum size (%d) allowed"), (int)covers.size(), (int)StructBlock::MaxFieldLevels);

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
			linkdefs.push_back( LinkDef( mi->range, width, linkar[ width-1].size()));
			linkar[ width-1].insert( linkar[ width-1].end(), lnkset.begin(), lnkset.end());
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
	return 	StructBlock( docno(), fieldar, linkbasear, linkar, enumar, repeatar, startar, pkbytear, pkshortar);
}




