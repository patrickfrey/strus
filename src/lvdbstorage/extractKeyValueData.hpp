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
#ifndef _STRUS_LVDB_EXTRACT_KEY_VALUE_DATA_HPP_INCLUDED
#define _STRUS_LVDB_EXTRACT_KEY_VALUE_DATA_HPP_INCLUDED
#include "databaseKey.hpp"
#include "metaDataDescription.hpp"
#include "metaDataBlock.hpp"
#include "docnoBlock.hpp"
#include "posinfoBlock.hpp"
#include "variant.hpp"
#include "strus/index.hpp"
#include <utility>
#include <vector>
#include <string>
#include <leveldb/db.h>

namespace strus {

struct TermTypeData
{
	const char* typestr;
	std::size_t typesize;
	Index typeno;

	TermTypeData( const leveldb::Slice& key, const leveldb::Slice& value);

	void print( std::ostream& out);
};

struct TermValueData
{
	const char* valuestr;
	std::size_t valuesize;
	Index valueno;

	TermValueData( const leveldb::Slice& key, const leveldb::Slice& value);

	void print( std::ostream& out);
};

struct DocIdData
{
	const char* docidstr;
	std::size_t docidsize;
	Index docno;

	DocIdData( const leveldb::Slice& key, const leveldb::Slice& value);

	void print( std::ostream& out);
};

struct ForwardIndexData
{
	Index docno;
	Index typeno;
	Index pos;
	const char* valuestr;
	std::size_t valuesize;

	ForwardIndexData( const leveldb::Slice& key, const leveldb::Slice& value);

	void print( std::ostream& out);
};

struct VariableData
{
	const char* varnamestr;
	std::size_t varnamesize;
	Index valueno;

	VariableData( const leveldb::Slice& key, const leveldb::Slice& value);

	void print( std::ostream& out);
};

struct DocMetaDataData
{
	Index blockno;
	std::vector<std::string> colnames;
	const MetaDataDescription* descr;
	MetaDataBlock block;

	DocMetaDataData( const MetaDataDescription* metadescr, const leveldb::Slice& key, const leveldb::Slice& value);

	void print( std::ostream& out);
};

struct DocAttributeData
{
	Index docno;
	Index attribno;
	const char* valuestr;
	unsigned int valuesize;

	DocAttributeData( const leveldb::Slice& key, const leveldb::Slice& value);

	void print( std::ostream& out);
};

struct DocFrequencyData
{
	Index typeno;
	Index termno;
	Index df;

	DocFrequencyData( const leveldb::Slice& key, const leveldb::Slice& value);

	void print( std::ostream& out);
};

struct DocnoBlockData
{
	Index typeno;
	Index termno;
	const DocnoBlockElement* blk;
	std::size_t blksize;

	DocnoBlockData( const leveldb::Slice& key, const leveldb::Slice& value);

	void print( std::ostream& out);
};

struct PosinfoBlockData
{
	Index typeno;
	Index valueno;
	struct PosinfoPosting
	{
		Index docno;
		std::vector<Index> pos;

		PosinfoPosting() :docno(0){}
		PosinfoPosting( const PosinfoPosting& o)
			:docno(o.docno),pos(o.pos){}
		PosinfoPosting( const Index& docno_, const std::vector<Index>& pos_)
			:docno(docno_),pos(pos_){}
	};
	std::vector<PosinfoPosting> posinfo;

	PosinfoBlockData( const leveldb::Slice& key, const leveldb::Slice& value);

	void print( std::ostream& out);
};

struct AttributeKeyData
{
	const char* varnamestr;
	std::size_t varnamesize;
	Index valueno;

	AttributeKeyData( const leveldb::Slice& key, const leveldb::Slice& value);

	void print( std::ostream& out);
};

struct MetaDataDescrData
{
	MetaDataDescription descr;

	MetaDataDescrData( const leveldb::Slice& key, const leveldb::Slice& value);
	void print( std::ostream& out);
};


}//namespace
#endif

