/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_DOCUMENT_UPDATE_HPP_INCLUDED
#define _STRUS_STORAGE_DOCUMENT_UPDATE_HPP_INCLUDED
#include "strus/storageDocumentUpdateInterface.hpp"
#include "strus/numericVariant.hpp"
#include "storageTransaction.hpp"
#include "storageDocumentStructs.hpp"
#include <vector>
#include <string>
#include <set>
#include <map>

namespace strus {
/// \brief Forward declaration
class Storage;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \class StorageDocumentUpdate
class StorageDocumentUpdate
	:public StorageDocumentUpdateInterface
{
public:
	/// \brief Constructor
	StorageDocumentUpdate(
		StorageTransaction* transaction_,
		const Index& docno_,
		ErrorBufferInterface* errorhnd_);

	/// \brief Destructor
	virtual ~StorageDocumentUpdate();

	/// \brief Implementation of StorageDocumentUpdateInterface::addSearchIndexTerm( const std::string&, const std::string&, const Index&);
	virtual void addSearchIndexTerm(
			const std::string& type_,
			const std::string& value_,
			const Index& position_);

	/// \brief Implementation of StorageDocumentUpdateInterface::addForwardIndexTerm( const std::string&, const std::string&, const Index&);
	virtual void addForwardIndexTerm(
			const std::string& type_,
			const std::string& value_,
			const Index& position_);

	/// \brief Implementation of StorageDocumentUpdateInterface::setMetaData( const std::string&, const NumericVariant&);
	virtual void setMetaData(
			const std::string& name_,
			const NumericVariant& value_);

	/// \brief Implementation of StorageDocumentUpdateInterface::setAttribute( const std::string&, const std::string&);
	virtual void setAttribute(
			const std::string& name_,
			const std::string& value_);

	/// \brief Implementation of StorageDocumentUpdateInterface::clearAttribute( const std::string&);
	virtual void clearAttribute(
			const std::string& name_);

	/// \brief Implementation of StorageDocumentUpdateInterface::setUserAccessRight( const std::string&);
	virtual void setUserAccessRight(
			const std::string& username_);

	/// \brief Implementation of StorageDocumentUpdateInterface::clearUserAccessRight( const std::string&);
	virtual void clearUserAccessRight(
			const std::string& username_);

	/// \brief Implementation of StorageDocumentUpdateInterface::clearUserAccessRights();
	virtual void clearUserAccessRights();

	/// \brief Implementation of StorageDocumentUpdateInterface::done();
	virtual void done();

public:
	const TermMap& terms() const				{return m_terms;}
	const InvMap& invs() const				{return m_invs;}

private:
	TermMapKey termMapKey( const std::string& type_, const std::string& value_);

private:
	StorageTransaction* m_transaction;			///< transaction
	Index m_docno;						///< document number
	TermMap m_terms;					///< map of all search index terms added
	InvMap m_invs;						///< map of all forward index terms added
	std::vector<DocAttribute> m_attributes;			///< attributes to update
	std::vector<DocMetaData> m_metadata;			///< metadata to update
	std::vector<Index> m_add_userlist;			///< list of users to add to this document access
	std::vector<Index> m_del_userlist;			///< list of users to remove from this document access
	bool m_doClearUserlist;					///< true if the list of all users should be cleared before this transaction
	ErrorBufferInterface* m_errorhnd;			///< error buffer for exception free interface
};

}
#endif


