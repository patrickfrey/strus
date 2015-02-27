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
class StorageClient;
/// \brief Forward declaration
class DatabaseClientInterface;

/// \class StorageDocumentChecker
class StorageDocumentChecker
	:public StorageDocumentInterface
{
public:
	StorageDocumentChecker(
		const StorageClient* storage_,
		const DatabaseClientInterface* database_,
		const std::string& docid_,
		const std::string& logfile_);

	virtual ~StorageDocumentChecker();

	virtual void addSearchIndexTerm(
			const std::string& type_,
			const std::string& value_,
			const Index& position_);

	virtual void addForwardIndexTerm(
			const std::string& type_,
			const std::string& value_,
			const Index& position_);

	virtual void setMetaData(
			const std::string& name_,
			const ArithmeticVariant& value_);

	virtual void setAttribute(
			const std::string& name_,
			const std::string& value_);

	virtual void setUserAccessRight( const std::string& username);

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

	struct InvKey
	{
		InvKey( const std::string& type_, const Index& pos_)
			:type(type_),pos(pos_){}
		InvKey( const InvKey& o)
			:type(o.type),pos(o.pos){}

		bool operator <( const InvKey& o) const
		{
			if (type < o.type) return true;
			if (type > o.type) return false;
			return (pos < o.pos);
		}

		std::string type;
		Index pos;
	};

	struct TermAttributes
	{
		explicit TermAttributes(){}
		TermAttributes( const TermAttributes& o)
			:poset(o.poset){}

		std::set<Index> poset;
	};

	typedef std::map<Term,TermAttributes> TermMap;
	typedef std::map<InvKey,std::string> InvTermMap;
	typedef std::map<std::string,ArithmeticVariant> MetaDataMap;
	typedef std::map<std::string,std::string> AttributeMap;

private:
	const StorageClient* m_storage;
	const DatabaseClientInterface* m_database;
	TermMap m_termMap;
	InvTermMap m_invTermMap;
	MetaDataMap m_metaDataMap;
	AttributeMap m_attributeMap;
	std::vector<std::string> m_userlist;
	std::string m_docid;
	Index m_docno;
	std::string m_logfile;
};

}
#endif


