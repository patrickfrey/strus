/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#ifndef _STRUS_LVDB_DOCNO_BLOCK_MAP_HPP_INCLUDED
#define _STRUS_LVDB_DOCNO_BLOCK_MAP_HPP_INCLUDED
#include "strus/index.hpp"
#include "docnoBlock.hpp"
#include "blockKey.hpp"
#include "blockMap.hpp"
#include "keyMap.hpp"
#include <cstdlib>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

namespace strus {

class DocnoBlockMap
	:public BlockMap<DocnoBlock,DocnoBlockElementMap>
{
public:
	typedef BlockMap<DocnoBlock,DocnoBlockElementMap> Parent;

public:
	DocnoBlockMap( leveldb::DB* db_)
		:BlockMap<DocnoBlock,DocnoBlockElementMap>(db_){}
	DocnoBlockMap( const DocnoBlockMap& o)
		:BlockMap<DocnoBlock,DocnoBlockElementMap>(o){}

	void defineDocnoPosting(
		const Index& termtype,
		const Index& termvalue,
		const Index& docno,
		unsigned int ff,
		float weight)
	{
		defineElement( BlockKey( termtype, termvalue), docno, DocnoBlockElement( docno, ff, weight));
	}

	void deleteDocnoPosting(
		const Index& termtype,
		const Index& termvalue,
		const Index& docno)
	{
		defineElement( BlockKey( termtype, termvalue), docno, DocnoBlockElement( docno, 0, 0.0));
	}

	class TermnoRenamer
	{
	public:
		TermnoRenamer( const std::map<Index,Index>* termnomap_)
			:m_termnomap(termnomap_)
		{}

		bool isCandidate( const BlockKeyIndex& keyidx) const
		{
			BlockKey key( keyidx);
			return (KeyMap::isUnknown( key.elem(2)));
		}
		BlockKeyIndex map( const BlockKeyIndex& keyidx) const
		{
			BlockKey oldkey( keyidx);
			std::map<Index,Index>::const_iterator mi = m_termnomap->find( oldkey.elem(2));
			if (mi == m_termnomap->end())
			{
				throw std::runtime_error( "internal: term value undefined (term number map)");
			}
			BlockKey newkey( oldkey.elem(1), mi->second);
			return newkey.index();
		}

	private:
		const std::map<Index,Index>* m_termnomap;
	};

	void renameNewTermNumbers( const std::map<Index,Index>& renamemap)
	{
		TermnoRenamer renamer( &renamemap);
		renameKeys( renamer);
	}

	void getWriteBatch( leveldb::WriteBatch& batch)
	{
		getWriteBatchMerge( batch);
	}
	
};

}
#endif

