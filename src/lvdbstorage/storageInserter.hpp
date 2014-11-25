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
#ifndef _STRUS_LVDB_STORAGE_INSERTER_HPP_INCLUDED
#define _STRUS_LVDB_STORAGE_INSERTER_HPP_INCLUDED
#include "strus/storageInserterInterface.hpp"
#include "strus/arithmeticVariant.hpp"
#include <vector>
#include <string>
#include <set>
#include <map>

namespace strus {
/// \brief Forward declaration
class Storage;

/// \class StorageInserter
class StorageInserter
	:public StorageInserterInterface
{
public:
	StorageInserter( Storage* storage_, const std::string& docid_);

	virtual ~StorageInserter();
	virtual void addTermOccurrence(
			const std::string& type_,
			const std::string& value_,
			const Index& position_,
			float weight_);

	virtual void setMetaData(
			const std::string& name_,
			const ArithmeticVariant& value_);

	virtual void setAttribute(
			const std::string& name_,
			const std::string& value_);

	virtual void done();

private:
	typedef std::pair<Index,Index> TermMapKey;
	struct TermMapValue
	{
		TermMapValue()
			:weight(0.0){}
		TermMapValue( const TermMapValue& o)
			:pos(o.pos),weight(o.weight){}


		std::set<Index> pos;
		float weight;
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

	TermMapKey termMapKey( const std::string& type_, const std::string& value_);

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
		ArithmeticVariant value;

		DocMetaData( const std::string& name_, const ArithmeticVariant& value_)
			:name(name_),value(value_){}
		DocMetaData( const DocMetaData& o)
			:name(o.name),value(o.value){}
	};

private:
	StorageInserter( const StorageInserter&){}	//non copyable
	void operator=( const StorageInserter&){}	//non copyable

private:
	Storage* m_storage;
	std::string m_docid;
	TermMap m_terms;
	InvMap m_invs;
	std::vector<DocAttribute> m_attributes;
	std::vector<DocMetaData> m_metadata;
};

}
#endif


