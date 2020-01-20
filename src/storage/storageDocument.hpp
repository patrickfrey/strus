/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_DOCUMENT_HPP_INCLUDED
#define _STRUS_STORAGE_DOCUMENT_HPP_INCLUDED
#include "strus/storageDocumentInterface.hpp"
#include "strus/storageDocumentUpdateInterface.hpp"
#include "strus/numericVariant.hpp"
#include "storageTransaction.hpp"
#include "storageDocumentStructs.hpp"
#include "structBlockBuilder.hpp"
#include <vector>
#include <string>
#include <set>
#include <map>

namespace strus {
/// \brief Forward declaration
class Storage;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \class StorageDocument
class StorageDocument
	:public StorageDocumentInterface
{
public:
	/// \brief Constructor
	StorageDocument(
		StorageTransaction* transaction_,
		const std::string& docid_,
		strus::Index docno_,
		ErrorBufferInterface* errorhnd_);

	/// \brief Destructor
	virtual ~StorageDocument();

	/// \brief Implementation of StorageDocumentInterface::addSearchIndexTerm( const std::string&, const std::string&, const Index&);
	virtual void addSearchIndexTerm(
			const std::string& type_,
			const std::string& value_,
			const Index& position_);

	/// \brief Implementation of StorageDocumentInterface::addSearchIndexStructure( const std::string&, const IndexRange&, const IndexRange&);
	virtual void addSearchIndexStructure(
			const std::string& struct_,
			const IndexRange& source_,
			const IndexRange& sink_);

	/// \brief Implementation of StorageDocumentInterface::addForwardIndexTerm( const std::string&, const std::string&, const Index&);
	virtual void addForwardIndexTerm(
			const std::string& type_,
			const std::string& value_,
			const Index& position_);

	/// \brief Implementation of StorageDocumentInterface::setMetaData( const std::string&, const NumericVariant&);
	virtual void setMetaData(
			const std::string& name_,
			const NumericVariant& value_);

	/// \brief Implementation of StorageDocumentInterface::setAttribute( const std::string&, const std::string&);
	virtual void setAttribute(
			const std::string& name_,
			const std::string& value_);

	/// \brief Implementation of StorageDocumentInterface::setUserAccessRights( const std::string&);
	virtual void setUserAccessRight(
			const std::string& username_);

	/// \brief Implementation of StorageDocumentInterface::done();
	virtual void done();

public:
	const std::string& docid() const			{return m_docid;}
	const Index& docno() const				{return m_docno;}
	const TermMap& terms() const				{return m_terms;}
	const InvMap& invs() const				{return m_invs;}
	const std::vector<DocAttribute>& attributes() const	{return m_attributes;}
	const std::vector<DocMetaData>& metadata() const	{return m_metadata;}
	const std::vector<Index>& userlist() const		{return m_userlist;}

private:
	TermMapKey termMapKey( const std::string& type_, const std::string& value_);

private:
#if __cplusplus >= 201103L
	StorageDocument( StorageDocument&) = delete;	//... non copyable
	void operator=( StorageDocument&) = delete;	//... non copyable
#endif

private:
	StorageTransaction* m_transaction;			///< transaction
	std::string m_docid;					///< document id assigned to this document
	Index m_docno;						///< document number
	TermMap m_terms;					///< map of all search index terms added
	InvMap m_invs;						///< map of all forward index terms added
	StructBlockBuilder m_structBuilder;			///< builder of structure block to add
	std::vector<DocAttribute> m_attributes;			///< attributes to add
	std::vector<DocMetaData> m_metadata;			///< metadata to add
	std::vector<Index> m_userlist;				///< users granted access to this document
	Index m_maxpos;						///< maximum position inserted
	int m_nofStructuresIgnored;				///< number of structures ignored
	ErrorBufferInterface* m_errorhnd;			///< error buffer for exception free interface
};

}
#endif


