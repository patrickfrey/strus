/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STRUCTURE_BLOCK_BUILDER_HPP_INCLUDED
#define _STRUS_STRUCTURE_BLOCK_BUILDER_HPP_INCLUDED
#include "dataBlock.hpp"
#include "skipScanArray.hpp"
#include "strus/constants.hpp"
#include "strus/index.hpp"
#include <vector>
#include <cstring>
#include <string>
#include <map>
#include <set>

namespace strus {

class StructBlockBuilder
{
public:
	struct StructureDeclaration
	{
		strus::Index docno;
		strus::Index structno;
		strus::IndexRange src;
		strus::IndexRange sink;

		StructureDeclaration( strus::Index docno_, strus::Index structno_, const strus::IndexRange& src_, const strus::IndexRange& sink_)
			:docno(docno_),structno(structno_),src(src_),sink(sink_){}
		StructureDeclaration( const StructureDeclaration& o)
			:docno(o.docno),structno(o.structno),src(o.src),sink(o.sink){}
	};

	StructBlockBuilder()
		:m_ar(),m_id(0),m_nofStructures(0),m_indexCount(0),m_lastSource(){}
	StructBlockBuilder( const StructBlockBuilder& o)
		:m_ar(o.m_ar)
		,m_id(o.m_id)
		,m_nofStructures(o.m_nofStructures)
		,m_indexCount(o.m_indexCount)
		,m_lastSource(o.m_lastSource){}

	StructBlockBuilder( const std::vector<StructureDeclaration>& declarations);

	Index id() const
	{
		return m_id;
	}
	void setId( Index id_);

	bool empty() const
	{
		return m_ar.empty();
	}
	Index lastDoc() const
	{
		return m_ar.empty() ? 0 : m_ar.back().docno;
	}
	Index firstDoc() const
	{
		return m_ar.empty() ? 0 : m_ar[ 0].docno;
	}

	struct StructureLink
	{
		bool head;
		strus::Index structno;
		int idx;

		StructureLink()
			:head(true),structno(0),idx(0){}
		StructureLink( bool head_, strus::Index structno_, int idx_)
			:head(head_),structno(structno_),idx(idx_){}
		StructureLink( const StructureLink& o)
			:head(o.head),structno(o.structno),idx(o.idx){}

		bool operator < (const StructureLink& o) const
		{
			return head == o.head
				? (structno == o.structno
					? (idx < o.idx)
					: (structno < o.structno))
				: (head == true);
		}
		bool valid() const
		{
			return structno && idx;
		}
		bool operator==( const StructureLink& o) const
		{
			return head == o.head && structno == o.structno && idx == o.idx;
		}
	};

	bool fitsInto( std::size_t nofNewStructures) const
	{
		int estimatedConsumption = (m_nofStructures + nofNewStructures) * sizeof(strus::IndexRange);
		return estimatedConsumption <= Constants::maxStructBlockSize();
	}

	bool full() const
	{
		return (int)(m_nofStructures * sizeof(strus::IndexRange)) >= Constants::maxStructBlockSize();
	}

	/// \brief Eval if the block is filled with a given ratio
	/// \param[in] ratio value between 0.0 and 1.0, reasonable is a value close to one
	/// \note A small value leads to fragmentation, a value close to 1.0 leads to transactions slowing down
	bool filledWithRatio( float ratio) const
	{
		return m_nofStructures >= (int)(ratio * Constants::maxStructBlockSize());
	}

	void clear()
	{
		m_ar.clear();
		m_id = 0;
		m_nofStructures = 0;
		m_indexCount = 0;
		m_lastSource = strus::IndexRange();
	}

	int size() const
	{
		return m_nofStructures;
	}

	/// \brief Add a new relation to the block
	bool append( strus::Index docno, strus::Index structno, const strus::IndexRange& src, const strus::IndexRange& sink);

	static void merge(
			StructBlockBuilder& blk1,
			StructBlockBuilder& blk2,
			StructBlockBuilder& newblk);

	static void split(
			StructBlockBuilder& blk,
			StructBlockBuilder& newblk1,
			StructBlockBuilder& newblk2);

	void swap( StructBlockBuilder& o)
	{
		m_ar.swap( o.m_ar);
		std::swap( m_id, o.m_id);
		std::swap( m_nofStructures, o.m_nofStructures);
		std::swap( m_indexCount, o.m_indexCount);
		std::swap( m_lastSource, o.m_lastSource);
	}

	std::vector<StructureDeclaration> declarations() const;

	void check() const;

private:
	struct DocStructureMap
	{
		typedef std::pair<strus::IndexRange,StructureLink> MapElement;
		typedef std::pair<StructureLink,strus::IndexRange> InvElement;
		typedef std::set<MapElement> Map;
		typedef std::set<InvElement> InvMap;

		strus::Index docno;
		Map map;
		InvMap invmap;

		explicit DocStructureMap( strus::Index docno_)
			:docno(docno_),map(),invmap(){}
		DocStructureMap( const DocStructureMap& o)
			:docno(o.docno),map(o.map),invmap(o.invmap){}

		struct SearchCompare
		{
			SearchCompare(){}
			bool operator()( const DocStructureMap& aa, strus::Index docno) const
			{
				return aa.docno <= docno;
			}
		};

		bool append( const strus::IndexRange& range, const StructureLink& link)
		{
			bool rt = false;
			rt |= map.insert( MapElement( range, link)).second;
			rt |= invmap.insert( InvElement( link, range)).second;
			return rt;
		}

		bool headerExists( const strus::IndexRange& range, strus::Index structno)
		{
			Map::const_iterator ri = first( range), re = end();
			for (; ri != re && ri->first == range; ++ri)
			{
				if (ri->second.head && ri->second.structno == structno) return true;
			}
			return false;
		}

		int maxIndex() const
		{
			int maxidx = 0;
			InvMap::const_iterator ri = inv_begin(), re = inv_end();
			for (; ri != re && ri->first.head; ++ri)
			{
				if (ri->first.idx > maxidx) maxidx = ri->first.idx;
			}
			return maxidx;
		}

		strus::IndexRange lastSource() const
		{
			if (map.empty()) return strus::IndexRange();
			Map::reverse_iterator ri = map.rend(), re = map.rbegin();
			for (; ri != re && !ri->second.head; ++ri){}
			return (ri != re) ? ri->first : strus::IndexRange();
		}

		typedef Map::const_iterator const_iterator;
		typedef InvMap::const_iterator inv_iterator;

		const_iterator begin() const	{return map.begin();}
		const_iterator end() const	{return map.end();}
		inv_iterator inv_begin() const	{return invmap.begin();}
		inv_iterator inv_end() const	{return invmap.end();}

		const_iterator first( const strus::IndexRange& range) const
		{
			return map.lower_bound( MapElement( range, StructureLink()));
		}
		inv_iterator inv_first( const StructureLink& link) const
		{
			return invmap.lower_bound( InvElement( link, strus::IndexRange()));
		}
	};

	class DocStructureScanner
	{
	public:
		DocStructureScanner( const DocStructureScanner& o)
			:m_ar(o.m_ar)
			,m_aridx(o.m_aridx)
		{}
		DocStructureScanner( const StructBlockBuilder& builder)
			:m_ar(builder.m_ar.data(),builder.m_ar.size())
			,m_aridx(0)
		{}

		strus::Index skipDoc( strus::Index docno);

		DocStructureMap::const_iterator begin() const	{return !m_ar.empty() ? m_ar[ m_aridx].begin() : DocStructureMap::const_iterator();}
		DocStructureMap::const_iterator end() const	{return !m_ar.empty() ? m_ar[ m_aridx].end() : DocStructureMap::const_iterator();}
		DocStructureMap::inv_iterator inv_begin() const	{return !m_ar.empty() ? m_ar[ m_aridx].inv_begin() : DocStructureMap::inv_iterator();}
		DocStructureMap::inv_iterator inv_end() const	{return !m_ar.empty() ? m_ar[ m_aridx].inv_end() : DocStructureMap::inv_iterator();}

	private:
		SkipScanArray<DocStructureMap,strus::Index,DocStructureMap::SearchCompare> m_ar;
		int m_aridx;
	};

	DocStructureScanner getDocStructureScanner() const
	{
		return DocStructureScanner( *this);
	}

private:
	std::vector<DocStructureMap> m_ar;
	strus::Index m_id;
	int m_nofStructures;
	int m_indexCount;
	strus::IndexRange m_lastSource;
};
}//namespace
#endif

