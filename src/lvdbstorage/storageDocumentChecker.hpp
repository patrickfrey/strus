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
#ifndef _STRUS_LVDB_DOCUMENT_CHECKER_HPP_INCLUDED
#define _STRUS_LVDB_DOCUMENT_CHECKER_HPP_INCLUDED
#include "strus/storageDocumentInterface.hpp"
#include "strus/arithmeticVariant.hpp"
#include <string>
#include <vector>
#include <set>
#include <map>
#include <iostream>

namespace strus {
/// \brief Forward declaration
class Storage;

/// \class StorageDocumentChecker
class StorageDocumentChecker
	:public StorageDocumentInterface
{
public:
	StorageDocumentChecker(
		Storage* storage_,
		const std::string& docid_,
		const std::string& logfile_);

	virtual ~StorageDocumentChecker();

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
	void doCheck( std::ostream& logout);

private:
	StorageDocumentChecker( const StorageDocumentChecker&){}//non copyable
	void operator=( const StorageDocumentChecker&){}	//non copyable

	struct Term
	{
		Term( const std::string& type_, const std::string& value_)
			:type(type_),value(value_){}
		Term( const Term& o)
			:type(o.type),value(o.value){}

		bool operator <( const Term& o) const
		{
			if (type < o.type) return true;
			if (type > o.type) return false;
			return (value < o.value);
		}

		std::string type;
		std::string value;
	};

	struct TermAttributes
	{
		explicit TermAttributes( float weight_=0.0)
			:weight(weight_){}
		TermAttributes( const TermAttributes& o)
			:poset(o.poset),weight(o.weight){}

		std::set<Index> poset;
		float weight;
	};

	typedef std::map<Term,TermAttributes> TermMap;
	typedef std::map<std::string,ArithmeticVariant> MetaDataMap;
	typedef std::map<std::string,std::string> AttributeMap;

private:
	Storage* m_storage;
	TermMap m_termMap;
	MetaDataMap m_metaDataMap;
	AttributeMap m_attributeMap;
	std::string m_docid;
	Index m_docno;
	std::string m_logfile;
};

}
#endif


