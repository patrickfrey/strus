/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Structure for storing similarity relations
#ifndef _STRUS_VECTOR_SPACE_MODEL_SIMILARITY_RELATION_MAP_HPP_INCLUDED
#define _STRUS_VECTOR_SPACE_MODEL_SIMILARITY_RELATION_MAP_HPP_INCLUDED
#include "simHash.hpp"
#include "simGroup.hpp"
#include "strus/index.hpp"
#include "strus/base/stdint.h"
#include <vector>
#include <map>

namespace strus {

/// \brief Structure for storing sample similarity relations
class SimRelationMap
{
public:
	struct Element
	{
		SampleIndex index;
		unsigned short simdist;

		Element()
			:index(0),simdist(0){}
		Element( SampleIndex index_, unsigned short simdist_)
			:index(index_),simdist(simdist_){}
		Element( const Element& o)
			:index(o.index),simdist(o.simdist){}

		bool operator < (const Element& o) const
		{
			if (simdist == o.simdist) return index < o.index;
			return simdist < o.simdist;
		}
	};

public:
	SimRelationMap()
		:m_ar(),m_rowdescrmap(){}
	SimRelationMap( const SimRelationMap& o)
		:m_ar(o.m_ar),m_rowdescrmap(o.m_rowdescrmap){}

	class Row
	{
	public:
		typedef std::vector<Element>::const_iterator const_iterator;

		Row( const const_iterator& begin_, const const_iterator& end_)
			:m_begin(begin_),m_end(end_){}

		const_iterator begin() const	{return m_begin;}
		const_iterator end() const	{return m_end;}

	private:
		const_iterator m_begin;
		const_iterator m_end;
	};

	/// \brief Get a row declared by index
	Row row( const SampleIndex& index) const
	{
		RowDescrMap::const_iterator ri = m_rowdescrmap.find( index);
		if (ri == m_rowdescrmap.end()) return Row( m_ar.end(), m_ar.end());
		return Row( m_ar.begin() + ri->second.aridx, m_ar.begin() + ri->second.aridx + ri->second.size);
	}

	/// \brief Get this map as string
	std::string tostring() const;

	/// \brief Add a row reclaring the samples related (similar) to a sample
	/// \param[in] index index of sample to declare related elements of
	/// \param[in] ar array of samples with similarity distance related to the element referenced by 'index'
	void addRow( const SampleIndex& index, const std::vector<Element>& ar);

private:
	std::vector<Element> m_ar;
	struct RowDescr
	{
		std::size_t aridx;
		std::size_t size;

		RowDescr()
			:aridx(0),size(0){}
		RowDescr( std::size_t aridx_, std::size_t size_)
			:aridx(aridx_),size(size_){}
		RowDescr( const RowDescr& o)
			:aridx(o.aridx),size(o.size){}
	};
	typedef std::map<SampleIndex,RowDescr> RowDescrMap;
	RowDescrMap m_rowdescrmap;
};

}//namespace
#endif
