/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
///\brief Structure types needed for storage document and storage document update
///\file storageDocumentStructs.hpp
#ifndef _STRUS_STORAGE_DOCUMENT_STRUCTS_HPP_INCLUDED
#define _STRUS_STORAGE_DOCUMENT_STRUCTS_HPP_INCLUDED
#include "strus/numericVariant.hpp"
#include <vector>
#include <string>
#include <set>
#include <map>

namespace strus {

typedef std::pair<Index,Index> TermMapKey;
struct TermMapValue
{
	TermMapValue(){}
	TermMapValue( const TermMapValue& o)
		:pos(o.pos){}

	std::set<Index> pos;
};
typedef std::map< TermMapKey, TermMapValue> TermMap;

struct InvMapKey
{
	InvMapKey( const Index& t, const Index& p)
		:typeno(t),pos(p){}
	InvMapKey( const InvMapKey& o)
		:typeno(o.typeno),pos(o.pos){}

	bool operator<( const InvMapKey& o) const
	{
		return (typeno < o.typeno || (typeno == o.typeno && pos < o.pos));
	}

	Index typeno;
	Index pos;
};
typedef std::map<InvMapKey, std::string> InvMap;

struct DocStructure
{
	Index structno;
	IndexRange source;
	IndexRange sink;

	DocStructure( const Index& structno_, const IndexRange& source_, const IndexRange& sink_)
		:structno(structno_),source(source_),sink(sink_){}
	DocStructure( const DocStructure& o)
		:structno(o.structno),source(o.source),sink(o.sink){}
};

struct DocAttribute
{
	std::string name;
	std::string value;

	DocAttribute( const std::string& name_, const std::string& value_)
		:name(name_),value(value_){}
	DocAttribute( const DocAttribute& o)
		:name(o.name),value(o.value){}
};

struct DocMetaData
{
	std::string name;
	NumericVariant value;

	DocMetaData( const std::string& name_, const NumericVariant& value_)
		:name(name_),value(value_){}
	DocMetaData( const DocMetaData& o)
		:name(o.name),value(o.value){}
};

}//namspace
#endif


