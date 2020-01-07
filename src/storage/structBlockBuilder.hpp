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
#include "structBlock.hpp"
#include "structBlockDeclaration.hpp"
#include "structBlockLink.hpp"
#include "skipScanArray.hpp"
#include "strus/constants.hpp"
#include "strus/index.hpp"
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <set>

namespace strus {

class StructBlockBuilder
{
public:
	explicit StructBlockBuilder( strus::Index docno_=0)
		:m_map(),m_docno(docno_),m_indexCount(0),m_lastStructno(0),m_lastSource(){}
	StructBlockBuilder( const StructBlockBuilder& o)
		:m_map(o.m_map)
		,m_docno(o.m_docno)
		,m_indexCount(o.m_indexCount)
		,m_lastStructno(o.m_lastStructno)
		,m_lastSource(o.m_lastSource){}
	StructBlockBuilder( const StructBlock& blk);

	StructBlockBuilder( strus::Index docno_, const std::vector<StructBlockDeclaration>& declarations);

	Index docno() const
	{
		return m_docno;
	}
	void setDocno( Index docno_)
	{
		m_docno = docno_;
	}

	bool empty() const
	{
		return m_map.empty();
	}

	void clear()
	{
		m_map.clear();
		m_docno = 0;
		m_indexCount = 0;
		m_lastStructno = 0;
		m_lastSource = strus::IndexRange();
	}

	std::size_t size() const
	{
		return m_map.size();
	}

	/// \brief Add a new relation to the block
	bool append( strus::Index structno, const strus::IndexRange& src, const strus::IndexRange& sink);

	void swap( StructBlockBuilder& o)
	{
		m_map.swap( o.m_map);
		std::swap( m_docno, o.m_docno);
		std::swap( m_indexCount, o.m_indexCount);
		std::swap( m_lastStructno, o.m_lastStructno);
		std::swap( m_lastSource, o.m_lastSource);
	}

	std::vector<StructBlockDeclaration> declarations() const;

	void check() const;

	StructBlock createBlock();

	void print( std::ostream& out) const;

public:/*local functions*/
	struct IndexRangeLinkPair
	{
		strus::IndexRange range;
		StructBlockLink link;

		IndexRangeLinkPair( const strus::IndexRange& range_, const StructBlockLink& link_)
			:range(range_),link(link_){}
		IndexRangeLinkPair( const IndexRangeLinkPair& o)
			:range(o.range),link(o.link){}

		bool operator < (const IndexRangeLinkPair& o) const
		{
			return range == o.range ? link < o.link : range < o.range;
		}
	};

	struct LinkIndexRangePair
	{
		StructBlockLink link;
		strus::IndexRange range;

		LinkIndexRangePair()
			:link(),range(){}
		LinkIndexRangePair( const StructBlockLink& link_, const strus::IndexRange& range_)
			:link(link_),range(range_){}
		LinkIndexRangePair( const IndexRangeLinkPair& o)
			:link(o.link),range(o.range){}

		bool operator < (const LinkIndexRangePair& o) const
		{
			return link == o.link ? range < o.range : link < o.link;
		}
	};

	struct IndexRangeLinkMap
	{
		typedef std::set<IndexRangeLinkPair> Map;
		typedef std::set<LinkIndexRangePair> InvMap;

		Map map;
		InvMap invmap;

		explicit IndexRangeLinkMap()
			:map(),invmap(){}
		IndexRangeLinkMap( const IndexRangeLinkMap& o)
			:map(o.map),invmap(o.invmap){}

		bool append( const strus::IndexRange& range, const StructBlockLink& link)
		{
			bool rt = false;
			rt |= map.insert( IndexRangeLinkPair( range, link)).second;
			rt |= invmap.insert( LinkIndexRangePair( link, range)).second;
			return rt;
		}

		void erase( Map::const_iterator mi)
		{
			map.erase( IndexRangeLinkPair( mi->range, mi->link));
			invmap.erase( LinkIndexRangePair( mi->link, mi->range));
		}

		bool headerExists( const strus::IndexRange& range, strus::Index structno)
		{
			Map::const_iterator ri = first( range), re = end();
			for (; ri != re && ri->range == range; ++ri)
			{
				if (ri->link.head && ri->link.structno == structno) return true;
			}
			return false;
		}

		int maxIndex() const
		{
			int maxidx = 0;
			InvMap::const_iterator ri = inv_begin(), re = inv_end();
			for (; ri != re && ri->link.head; ++ri)
			{
				if (ri->link.idx > maxidx) maxidx = ri->link.idx;
			}
			return maxidx;
		}

		strus::IndexRange lastSource() const
		{
			if (map.empty()) return strus::IndexRange();
			Map::reverse_iterator ri = map.rend(), re = map.rbegin();
			for (; ri != re && !ri->link.head; ++ri){}
			return (ri != re) ? ri->range : strus::IndexRange();
		}

		bool empty() const
		{
			return map.empty();
		}

		std::size_t size() const
		{
			return map.size();
		}

		void clear()
		{
			map.clear();
			invmap.clear();
		}

		void swap( IndexRangeLinkMap& o)
		{
			map.swap( o.map);
			invmap.swap( o.invmap);
		}

		typedef Map::const_iterator const_iterator;
		typedef InvMap::const_iterator inv_iterator;

		const_iterator begin() const	{return map.begin();}
		const_iterator end() const	{return map.end();}
		inv_iterator inv_begin() const	{return invmap.begin();}
		inv_iterator inv_end() const	{return invmap.end();}

		const_iterator first( const strus::IndexRange& range) const
		{
			return map.lower_bound( IndexRangeLinkPair( range, StructBlockLink()));
		}
		inv_iterator inv_first( const StructBlockLink& link) const
		{
			return invmap.lower_bound( LinkIndexRangePair( link, strus::IndexRange()));
		}
	};

	typedef std::set<strus::IndexRange> FieldCover;

private:
	std::vector<FieldCover> getFieldCovers() const;

private:
	IndexRangeLinkMap m_map;
	strus::Index m_docno;
	int m_indexCount;
	strus::Index m_lastStructno;
	strus::IndexRange m_lastSource;
};
}//namespace
#endif

