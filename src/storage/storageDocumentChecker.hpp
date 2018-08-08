/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_DOCUMENT_CHECKER_HPP_INCLUDED
#define _STRUS_STORAGE_DOCUMENT_CHECKER_HPP_INCLUDED
#include "strus/storageDocumentInterface.hpp"
#include "strus/numericVariant.hpp"
#include "strus/index.hpp"
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
/// \brief Forward declaration
class ErrorBufferInterface;

/// \class StorageDocumentChecker
class StorageDocumentChecker
	:public StorageDocumentInterface
{
public:
	StorageDocumentChecker(
		const StorageClient* storage_,
		const DatabaseClientInterface* database_,
		const std::string& docid_,
		const std::string& logfile_,
		ErrorBufferInterface* errorhnd_);

	virtual ~StorageDocumentChecker();

	virtual void addSearchIndexTerm(
			const std::string& type_,
			const std::string& value_,
			const Index& position_);

	virtual void addSearchIndexStructure(
			const std::string& struct_,
			const IndexRange& source_,
			const IndexRange& sink_);

	virtual void addForwardIndexTerm(
			const std::string& type_,
			const std::string& value_,
			const Index& position_);

	virtual void setMetaData(
			const std::string& name_,
			const NumericVariant& value_);

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

	struct Structure
	{
		std::string structname;
		IndexRange source;
		IndexRange sink;
	
		Structure( const std::string& structname_, const IndexRange& source_, const IndexRange& sink_)
			:structname(structname_),source(source_),sink(sink_){}
		Structure( const Structure& o)
			:structname(o.structname),source(o.source),sink(o.sink){}

		bool operator < ( const Structure& o) const
		{
			if (structname < o.structname) return true;
			if (structname > o.structname) return false;
			if (source.end() < o.source.end()) return true;
			if (source.end() > o.source.end()) return false;
			if (sink.end() < o.sink.end()) return true;
			if (sink.end() > o.sink.end()) return false;
			if (source.start() < o.source.start()) return true;
			if (source.start() > o.source.start()) return false;
			return (sink.start() < o.sink.start());
		}
	};

	typedef std::map<Term,TermAttributes> TermMap;
	typedef std::map<InvKey,std::string> InvTermMap;
	typedef std::map<std::string,NumericVariant> MetaDataMap;
	typedef std::map<std::string,std::string> AttributeMap;
	typedef std::vector<Structure> StructureList;

private:
	const StorageClient* m_storage;
	const DatabaseClientInterface* m_database;
	TermMap m_termMap;
	InvTermMap m_invTermMap;
	MetaDataMap m_metaDataMap;
	AttributeMap m_attributeMap;
	StructureList m_structurelist;
	std::vector<std::string> m_userlist;
	std::string m_docid;
	Index m_docno;
	std::string m_logfile;
	ErrorBufferInterface* m_errorhnd;			///< error buffer for exception free interface
};

}
#endif


