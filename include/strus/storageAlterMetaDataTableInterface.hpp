/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface to declare changes in the meta data table structure of the storage
/// \file "storageAlterMetaDataTableInterface.hpp"
#ifndef _STRUS_STORAGE_ALTER_META_DATA_TABLE_INTERFACE_HPP_INCLUDED
#define _STRUS_STORAGE_ALTER_META_DATA_TABLE_INTERFACE_HPP_INCLUDED
#include <string>
#include "strus/arithmeticVariant.hpp"

namespace strus
{

/// \class StorageAlterMetaDataTableInterface
/// \brief Interface to declare changes in the meta data table structure of the storage
class StorageAlterMetaDataTableInterface
{
public:
	/// \brief Destructor that is the doing the rollback too, if commit() was not called before
	virtual ~StorageAlterMetaDataTableInterface(){}

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

	/// \brief Declare a meta data element to be reset (all values are reinitialized with their default)
	/// \param[in] name name of the element values to reset
	virtual void clearElement(
			const std::string& name)=0;

	/// \brief Write the changes of the meta data table structure to the storage
	virtual bool commit()=0;

	/// \brief Rollback of the meta data table changes, no changes made
	virtual void rollback()=0;
};

}//namespace
#endif


