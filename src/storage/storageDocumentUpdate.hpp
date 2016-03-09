/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
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


