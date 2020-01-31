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
#include "strus/constants.hpp"
#include "strus/index.hpp"
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <set>

namespace strus {
/// \brief Forward declaration
class ErrorBufferInterface;

class StructBlockBuilder
{
public:
	explicit StructBlockBuilder( const std::string& docid_, strus::Index docno_, ErrorBufferInterface* errorhnd_)
		:m_map()
		,m_headerar()
		,m_docid(docid_)
		,m_docno(docno_)
		,m_indexCount(0)
		,m_errorhnd(errorhnd_){}
	StructBlockBuilder( const StructBlockBuilder& o)
		:m_map(o.m_map)
		,m_headerar(o.m_headerar)
		,m_docid(o.m_docid)
		,m_docno(o.m_docno)
		,m_indexCount(o.m_indexCount)
		,m_errorhnd(o.m_errorhnd){}

	StructBlockBuilder( const StructBlock& blk, const std::string& docid_, ErrorBufferInterface* errorhnd_);

	StructBlockBuilder( const std::string& docid_, strus::Index docno_, const std::vector<StructBlockDeclaration>& declarations, ErrorBufferInterface* errorhnd_);

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

		bool append( const strus::IndexRange& range, const StructBlockLink& link);

		void erase( Map::const_iterator mi);

		/// \param[in] structno structure type number
		/// \param[in] idx index assigned to structure
		/// \param[in,out] deletes where to append deleted structure declarations
		void removeStructure( strus::Index structno, unsigned int idx, std::vector<StructBlockDeclaration>& deletes);

		int findStructureHeader( const strus::IndexRange& range, strus::Index structno);

		int maxIndex() const;

		strus::IndexRange lastSource() const;

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
		std::vector<strus::IndexRange> fields() const
		{
			std::vector<strus::IndexRange> rt;
			const_iterator mi = map.begin(), me = map.end();
			for (; mi != me; ++mi)
			{
				if (rt.empty() || rt.back() != mi->range)
				{
					rt.push_back( mi->range);
				}
			}
			return rt;
		}
	};

	typedef std::set<strus::IndexRange> FieldCover;

private:
	std::vector<FieldCover> getFieldCovers() const;
	void eliminateStructuresBeyondCapacity();

private:
	IndexRangeLinkMap m_map;
	std::vector<StructBlock::PositionType> m_headerar;
	std::string m_docid;
	strus::Index m_docno;
	int m_indexCount;
	ErrorBufferInterface* m_errorhnd;
};
}//namespace
#endif

