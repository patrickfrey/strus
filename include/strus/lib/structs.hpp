/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Exported functions for handling structures in the storage
/// \file structs.hpp
#ifndef _STRUS_STORAGE_STRUCTS_LIB_HPP_INCLUDED
#define _STRUS_STORAGE_STRUCTS_LIB_HPP_INCLUDED
#include "strus/index.hpp"
#include <utility>
#include <vector>
#include <map>

/// \brief strus toplevel namespace
namespace strus {

/// \brief Forward declaration
class ErrorBufferInterface;
/// \brief Forward declaration
class StructIteratorInterface;

/// \brief Map of all structures defined in a document
class StorageStructMap
{
public:
	class Key
	{
	public:
		strus::Index structno() const	{return m_structno;}
		int index() const		{return m_index;}

		Key()
			:m_structno(0),m_index(0){}
		Key( strus::Index structno_, int index_)
			:m_structno(structno_),m_index(index_){}
		Key( const Key& o)
			:m_structno(o.m_structno),m_index(o.m_index){}

		bool operator < (const Key& o) const
		{
			return m_structno == o.m_structno ? m_index < o.m_index : m_structno < o.m_structno;
		}

	private:
		strus::Index m_structno;
		int m_index;
	};

	typedef std::pair<strus::IndexRange,std::vector<strus::IndexRange> > FieldRelation;
	typedef std::map<Key, FieldRelation> Map;
	typedef Map::const_iterator const_iterator;

	/// \brief Constructor
	/// \param[in] structitr structure iterator interface
	/// \param[in] errorhnd reference to error buffer (ownership hold by caller)
	StorageStructMap( StructIteratorInterface* structitr, strus::Index docno, ErrorBufferInterface* errorhnd);

	const_iterator begin() const				{return m_map.begin();}
	const_iterator end() const				{return m_map.end();}

	const_iterator begin( strus::Index structno) const	{return m_map.lower_bound( Key( structno,0));}
	const_iterator end( strus::Index structno) const	{return m_map.lower_bound( Key( structno+1,0));}

	bool empty() const					{return m_map.empty();}

private:
	Map m_map;
};

}//namespace
#endif

