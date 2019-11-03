/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface provided by the storage transaction interface to declare changes in the meta data table structure of the storage.
/// \file "storageMetaDataTableUpdateInterface.hpp"
#ifndef _STRUS_STORAGE_META_DATA_TABLE_UPDATE_INTERFACE_HPP_INCLUDED
#define _STRUS_STORAGE_META_DATA_TABLE_UPDATE_INTERFACE_HPP_INCLUDED
#include <string>

namespace strus
{

/// \class StorageMetaDataTableUpdateInterface
/// \brief Interface to declare changes in the meta data table structure of the storage
/// \note Currently it is not possible (disallowed by a flag) to update the meta data table structure and update document contents in the storage within the same transaction, you have to use a transaction to alter the one or the other. The reason for this is that we cannot guarantee that it works, therefore it is forbidden.
class StorageMetaDataTableUpdateInterface
{
public:
	/// \brief Destructor that is the doing the rollback too, if commit() was not called before
	virtual ~StorageMetaDataTableUpdateInterface(){}

	/// \brief Declare a new meta data element
	/// \param[in] name name of the element to add
	/// \param[in] datatype type of the element to add
	virtual void addElement(
			const std::string& name,
			const std::string& datatype)=0;

	/// \brief Declare a meta data element to be changed
	/// \param[in] oldname previous name of the element
	/// \param[in] name new name of the element
	/// \param[in] datatype new type of the element
	virtual void alterElement(
			const std::string& oldname,
			const std::string& name,
			const std::string& datatype)=0;

	/// \brief Declare a meta data element to be renamed
	/// \param[in] oldname previous name of the element
	/// \param[in] name new name of the element
	virtual void renameElement(
			const std::string& oldname,
			const std::string& name)=0;

	/// \brief Declare a meta data element to be deleted
	/// \param[in] name name of the element to delete
	virtual void deleteElement(
			const std::string& name)=0;

	/// \brief Declare all meta data elements to be deleted
	virtual void deleteElements()=0;
	
	/// \brief Declare a meta data element to be reset (all values are reinitialized with their default)
	/// \param[in] name name of the element values to reset
	virtual void clearElement(
			const std::string& name)=0;

	/// \brief Transmit all changes made into the transaction context
	virtual bool done()=0;
};

}//namespace
#endif


