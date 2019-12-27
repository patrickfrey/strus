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

namespace strus {

class StructBlockBuilder
{
public:
	StructBlockBuilder()
		:m_ar(),m_id(0),m_nofStructures(0){}
	StructBlockBuilder( const StructBlockBuilder& o)
		:m_ar(o.m_ar)
		,m_id(o.m_id)
		,m_nofStructures(o.m_nofStructures){}

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

	struct StructDeclaration
	{
		Index docno;
		IndexRange src;
		IndexRange sink;

		StructDeclaration( Index docno_, const IndexRange& src_, const IndexRange& sink_)
			:docno(docno_),src(src_),sink(sink_){}
		StructDeclaration( const StructDeclaration& o)
			:docno(o.docno),src(o.src),sink(o.sink){}
	};

	/// \brief Add a new structure relation to the block
	void append( Index docno, const strus::IndexRange& src, const strus::IndexRange& sink);

	bool fitsInto( std::size_t nofNewStructures) const
	{
		int estimatedConsumption = (m_nofStructures + nofNewStructures) * sizeof(strus::IndexRange);
		return estimatedConsumption <= Constants::maxStructBlockSize();
	}

	bool full() const
	{
		return m_nofStructures * sizeof(StructureMember) >= Constants::maxStructBlockSize();
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
	}

	int size() const;

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
	}

private:
	void addNewDocument( Index docno);
	void addLastDocStructure( const strus::IndexRange& src);
	void addLastStructureMemberRange( const strus::IndexRange& sink);

private:
	struct DocStructureMap
	{
		strus::Index docno;
		std::map<strus::IndexRange,int> map;

		DocStructureMap( strus::Index docno_, const std::map<strus::IndexRange,int>& map_)
			:docno(docno_),map(map_){}
		DocStructureMap( const DocStructureMap& o)
			:docno(o.docno),map(o.map){}

		struct SearchCompare
		{
			SearchCompare(){}
			bool operator()( const DocStructureMap& aa, const strus::Index& docno) const
			{
				return aa.docno <= docno;
			}
		};

		typedef std::map<strus::IndexRange,int>::const_iterator const_iterator;
		typedef std::map<strus::IndexRange,int>::iterator iterator;

		const_iterator begin() const	{return map.begin();}
		iterator begin()		{return map.begin();}
		const_iterator end() const	{return map.end();}
		iterator end()			{return map.end();}
	};

	class DocStructureScanner
	{
	public:
		DocStructureScanner( const DocStructureScanner& o)
			:m_ar(o.m_ar)
			,m_aridx(o.m_aridx)
		{}
		DocStructureScanner( const StructBlockBuilder& builder)
			:m_ar(&builder.m_ar.data(),builder.m_ar.size())
			,m_aridx(0)
		{}

		strus::Index skipDoc( strus::Index docno);

		DocStructureMap::const_iterator begin() const	{return m_aridx < m_ar.size() ? m_ar[ m_aridx].begin() : DocStructureMap::const_iterator();}
		DocStructureMap::const_iterator end() const	{return m_aridx < m_ar.size() ? m_ar[ m_aridx].end() : DocStructureMap::const_iterator();}

	private:
		SkipScanArray<DocStructureMap,strus::Index,DocStructureMap::SearchCompare> m_ar;
		int m_aridx;
	};

	DocStructureScanner getDocStructureScanner() const
	{
		return Scanner( *this);
	}

private:
	std::vector<DocStructureMap> m_ar;
	strus::Index m_id;
	int m_nofStructures;
};
}//namespace
#endif

