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
#ifndef _STRUS_LVDB_METADATA_BLOCK_HPP_INCLUDED
#define _STRUS_LVDB_METADATA_BLOCK_HPP_INCLUDED
#include "strus/index.hpp"
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

namespace strus {

class MetaDataBlock
{
public:
	MetaDataBlock( leveldb::DB* db_, Index blockno_, char varname_);
	MetaDataBlock( const MetaDataBlock& o);

	virtual ~MetaDataBlock(){}

	static std::size_t index( Index docno)			{return docno & MetaDataBlockMask;}
	static Index blockno( Index docno)			{return (docno>>MetaDataBlockShift)+1;}
	Index blockno() const					{return m_blockno;}

	void setValue( Index docno, float value);
	float getValue( Index docno) const;

	void addToBatch( leveldb::WriteBatch& batch);

private:
	enum {
		MetaDataBlockSize=1024,
		MetaDataBlockMask=1023,
		MetaDataBlockShift=10
	};
	void readBlockFromDB();

private:
	leveldb::DB* m_db;
	Index m_blockno;
	char m_varname;
	float m_blk[ MetaDataBlockSize];
};

}
#endif

