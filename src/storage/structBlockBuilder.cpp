/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "structBlockBuilder.hpp"
#include "private/internationalization.hpp"
#include <cstring>
#include <limits>

using namespace strus;

StructBlockBuilder::StructBlockBuilder( const std::vector<StructureDeclaration>& declarations)
	:m_ar(),m_id(0),m_nofStructures(0),m_indexCount(0),m_lastSource()
{
	std::vector<StructureDeclaration>::const_iterator di = declarations.begin(), de = declarations.end();
	for (; di != de; ++di)
	{
		(void)append( di->docno, di->structno, di->src, di->sink);
	}
}

strus::Index StructBlockBuilder::DocStructureScanner::skipDoc( strus::Index docno)
{
	if (m_ar.empty()) return 0;
	int idx = 0;
	if (docno <= m_ar[ m_aridx].docno)
	{
		if (docno == m_ar[ m_aridx].docno)
		{
			return docno;
		}
		idx = m_ar.upperbound( docno, 0, m_aridx+1);
	}
	else
	{
		idx = m_ar.upperbound( docno, m_aridx+1, m_ar.size());
	}
	return (idx >= 0) ? m_ar[ m_aridx = idx].docno : 0;
}

bool StructBlockBuilder::append( strus::Index docno, strus::Index structno, const strus::IndexRange& src, const strus::IndexRange& sink)
{
	if (m_ar.empty() || docno > m_ar.back().docno)
	{
		m_ar.push_back( DocStructureMap( docno));
		m_lastSource = strus::IndexRange();
		m_indexCount = 0;
	}
	else if (docno < m_ar.back().docno)
	{
		throw strus::runtime_error(_TXT("documents not added in ascending order to structure block (%d < %d)"), (int)docno, (int)m_ar.back().docno);
	}
	if (!src.defined() || !sink.defined())
	{
		throw strus::runtime_error(_TXT("relation with undefined source or sink added to structure block"));
	}
	if (0>structno)
	{
		throw strus::runtime_error(_TXT("relation with bad structure id added to structure block"));
	}
	if (src != m_lastSource)
	{
		++m_indexCount;
		m_lastSource = src;
		if (m_ar.back().headerExists( src, structno))
		{
			throw strus::runtime_error(_TXT("members of relation not appended to structure block after header"));
		}
	}
	bool rt = false;
	rt |= m_ar.back().append( src, StructureLink( true, structno, m_indexCount));
	rt |= m_ar.back().append( sink, StructureLink( false, structno, m_indexCount));
	if (rt) ++m_nofStructures;
	return rt;
}

std::vector<StructBlockBuilder::StructureDeclaration> StructBlockBuilder::declarations() const
{
	std::vector<StructureDeclaration> rt;
	std::vector<DocStructureMap>::const_iterator ai = m_ar.begin(), ae = m_ar.end();
	for (; ai != ae; ++ai)
	{
		DocStructureMap::inv_iterator si = ai->inv_begin(), se = ai->inv_end();
		for (; si != se && si->first.head; ++si)
		{
			StructureLink memberLink( false, si->first.structno, si->first.idx);
			DocStructureMap::inv_iterator mi = ai->inv_first( memberLink);
			for (; si->first == memberLink; ++si)
			{
				rt.push_back( StructureDeclaration( ai->docno, si->first.structno, si->second/*src*/, mi->second/*sink*/));
			}
		}
	}
	return rt;
}

void StructBlockBuilder::check() const
{
	int nofDeclarations = declarations().size();
	if (size() != nofDeclarations)
	{
		throw strus::runtime_error(_TXT("number of structures do not match (%d != %d)"), size(), nofDeclarations);
	}
	std::vector<DocStructureMap>::const_iterator ai = m_ar.begin(), ae = m_ar.end();
	for (; ai != ae; ++ai)
	{
		DocStructureMap::const_iterator si = ai->begin(), se = ai->end();
		for (; si != se; ++si)
		{
			DocStructureMap::const_iterator sn = si;
			for (++sn; sn != se; ++sn)
			{
				if (sn->first.overlap( si->first))
				{
					if (sn->second.head && si->second.head
					&&  sn->second.structno == si->second.structno)
					{
						throw strus::runtime_error(_TXT("structure headers are overlapping"));
					}
					if (!sn->second.head && !si->second.head
					&&  sn->second.structno == si->second.structno
					&&  sn->second.idx == si->second.idx)
					{
						throw strus::runtime_error(_TXT("structure members are overlapping"));
					}
				}
			}
		}
	}
}

void StructBlockBuilder::merge(
		StructBlockBuilder& blk1,
		StructBlockBuilder& blk2,
		StructBlockBuilder& newblk)
{
	std::vector<DocStructureMap>::const_iterator ai = blk1.m_ar.begin(), ae = blk1.m_ar.end();
	std::vector<DocStructureMap>::const_iterator bi = blk2.m_ar.begin(), be = blk2.m_ar.end();
	if (newblk.lastDoc() > blk1.lastDoc() || newblk.lastDoc() > blk2.lastDoc())
	{
		throw std::runtime_error(_TXT("cannot merge blocks into overlapping block"));
	}
	while (ai != ae && bi != be)
	{
		if (ai->docno < bi->docno)
		{
			newblk.m_ar.push_back( *ai);
		}
		else if (ai->docno > bi->docno)
		{
			newblk.m_ar.push_back( *bi);
		}
		else
		{
			throw std::runtime_error(_TXT("cannot merge overlapping blocks"));
		}
	}
	while (ai != ae)
	{
		newblk.m_ar.push_back( *ai);
		newblk.m_lastSource = newblk.m_ar.back().lastSource();
		newblk.m_indexCount = newblk.m_ar.back().maxIndex();
	}
	while (bi != be)
	{
		newblk.m_ar.push_back( *bi);
		newblk.m_lastSource = newblk.m_ar.back().lastSource();
		newblk.m_indexCount = newblk.m_ar.back().maxIndex();
	}
	newblk.m_nofStructures += blk1.m_nofStructures + blk2.m_nofStructures;
}

void StructBlockBuilder::split(
		StructBlockBuilder& blk,
		StructBlockBuilder& newblk1,
		StructBlockBuilder& newblk2)
{
	newblk1.clear();
	newblk2.clear();
	if (blk.empty()) return;

	std::vector<StructBlockBuilder::StructureDeclaration> dl = blk.declarations();
	int mid = dl.size() / 2;
	strus::Index split_docno = dl[ mid].docno;
	std::vector<StructBlockBuilder::StructureDeclaration>::const_iterator splititr;

	std::vector<StructBlockBuilder::StructureDeclaration>::const_iterator
		di = dl.begin(), de = dl.end(), dn = dl.begin() + mid + 1, dp = dl.begin() + mid;

	for (; dn != de && dp != di && split_docno == dn->docno && split_docno == dp->docno; ++dn,--dp){}
	if (dn == de || split_docno == dn->docno)
	{
		for (; dp != di && dp->docno == split_docno; --dp){}
		if (dp->docno == split_docno)
		{
			newblk1 = blk;
			return;
		}
		else
		{
			splititr = ++dp;
		}
	}
	else
	{
		splititr = dn;
	}
	for (; di != splititr; ++di)
	{
		newblk1.append( di->docno, di->structno, di->src, di->sink);
	}
	for (; di != de; ++di)
	{
		newblk2.append( di->docno, di->structno, di->src, di->sink);
	}
}


