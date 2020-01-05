/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_EXTRACT_KEY_VALUE_DATA_HPP_INCLUDED
#define _STRUS_STORAGE_EXTRACT_KEY_VALUE_DATA_HPP_INCLUDED
#include "strus/databaseCursorInterface.hpp"
#include "metaDataDescription.hpp"
#include "metaDataBlock.hpp"
#include "posinfoBlock.hpp"
#include "invTermBlock.hpp"
#include "strus/index.hpp"
#include <utility>
#include <vector>
#include <string>

namespace strus {

std::string extractKeyString( const strus::DatabaseCursorInterface::Slice& key);


struct TermTypeData
{
	const char* typestr;
	std::size_t typesize;
	Index typeno;

	TermTypeData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value);

	void print( std::ostream& out);
};

struct TermValueData
{
	const char* valuestr;
	std::size_t valuesize;
	Index valueno;

	TermValueData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value);

	void print( std::ostream& out);
};

struct StructTypeData
{
	const char* structnamestr;
	std::size_t structnamesize;
	Index structno;

	StructTypeData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value);

	void print( std::ostream& out);
};

struct DocIdData
{
	const char* docidstr;
	std::size_t docidsize;
	Index docno;

	DocIdData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value);

	void print( std::ostream& out);
};

struct UserNameData
{
	const char* usernamestr;
	std::size_t usernamesize;
	Index userno;

	UserNameData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value);

	void print( std::ostream& out);
};

struct TermTypeInvData
{
	const char* typestr;
	std::size_t typesize;
	Index typeno;

	TermTypeInvData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value);

	void print( std::ostream& out);
};

struct TermValueInvData
{
	const char* valuestr;
	std::size_t valuesize;
	Index valueno;

	TermValueInvData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value);

	void print( std::ostream& out);
};

struct ForwardIndexData
{
	Index docno;
	Index typeno;
	Index pos;

	struct Element
	{
		Index pos;
		std::string value;

		Element() :pos(0){}
		Element( const Element& o)
			:pos(o.pos),value(o.value){}
		Element( const Index& pos_, const std::string& value_)
			:pos(pos_),value(value_){}
	};
	std::vector<Element> elements;

	ForwardIndexData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value, std::size_t advsize=100);

	void print( std::ostream& out);
};

struct VariableData
{
	const char* varnamestr;
	std::size_t varnamesize;
	Index valueno;

	VariableData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value);

	void print( std::ostream& out);
};

struct DocMetaDataData
{
	Index blockno;
	std::vector<std::string> colnames;
	const MetaDataDescription* descr;
	MetaDataBlock block;

	DocMetaDataData( const MetaDataDescription* metadescr, const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value);

	void print( std::ostream& out);
};

struct DocAttributeData
{
	Index docno;
	Index attribno;
	const char* valuestr;
	unsigned int valuesize;

	DocAttributeData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value);

	void print( std::ostream& out);
};

struct DocFrequencyData
{
	Index typeno;
	Index termno;
	Index df;

	DocFrequencyData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value);

	void print( std::ostream& out);
};

struct PosinfoBlockData
{
	Index typeno;
	Index valueno;
	Index docno;

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

	PosinfoBlockData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value);

	void print( std::ostream& out);
};

struct StructBlockData
{
	Index docno;

	struct Structure
	{
		Index structno;
		IndexRange source;
		IndexRange sink;

		Structure( const Index& structno_, const IndexRange& source_, const IndexRange& sink_)
			:structno(structno_),source(source_),sink(sink_){}
		Structure( const Structure& o)
			:structno(o.structno),source(o.source),sink(o.sink){}
	};
	std::vector<Structure> structures;

	StructBlockData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value);

	void print( std::ostream& out);
};

struct StructTypeInvData
{
	const char* valuestr;
	std::size_t valuesize;
	Index valueno;

	StructTypeInvData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value);

	void print( std::ostream& out);
};

struct FfBlockData
{
	Index typeno;
	Index valueno;
	Index docno;

	struct Posting
	{
		Index docno;
		int ff;

		Posting( const Index& docno_, int ff_)
			:docno(docno_),ff(ff_){}
		Posting( const Posting& o)
			:docno(o.docno),ff(o.ff){}
	};
	std::vector<Posting> postings;

	FfBlockData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value);

	void print( std::ostream& out);
};

struct DocListBlockData
{
	Index typeno;
	Index valueno;
	Index docno;

	typedef std::pair<Index,Index> Range;
	std::vector<Range> docrangelist;

	DocListBlockData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value);

	void print( std::ostream& out);
};

struct InverseTermData
{
	typedef InvTermBlock::Element InvTerm;

	Index docno;
	std::vector<InvTerm> terms;

	InverseTermData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value);

	void print( std::ostream& out);
};

struct UserAclBlockData
{
	Index userno;
	Index docno;

	typedef std::pair<Index,Index> Range;
	std::vector<Range> docrangelist;

	UserAclBlockData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value);

	void print( std::ostream& out);
};

struct AclBlockData
{
	Index docno;
	Index userno;

	typedef std::pair<Index,Index> Range;
	std::vector<Range> userrangelist;

	AclBlockData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value);

	void print( std::ostream& out);
};

struct AttributeKeyData
{
	const char* varnamestr;
	std::size_t varnamesize;
	Index valueno;

	AttributeKeyData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value);

	void print( std::ostream& out);
};

struct MetaDataDescrData
{
	MetaDataDescription descr;

	MetaDataDescrData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value);
	void print( std::ostream& out);
};


}//namespace
#endif

