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
#include "strus/arithmeticVariant.hpp"
#include "storageTransaction.hpp"
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

	/// \brief Implementation of StorageDocumentUpdateInterface::setMetaData( const std::string&, const ArithmeticVariant&);
	virtual void setMetaData(
			const std::string& name_,
			const ArithmeticVariant& value_);

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

private:
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

	StorageTransaction* m_transaction;
	Index m_docno;
	std::vector<DocAttribute> m_attributes;
	std::vector<DocMetaData> m_metadata;
	std::vector<Index> m_add_userlist;
	std::vector<Index> m_del_userlist;
	bool m_doClearUserlist;
	ErrorBufferInterface* m_errorhnd;			///< error buffer for exception free interface
};

}
#endif


