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
#ifndef _STRUS_STORAGE_ALTER_META_DATA_TABLE_INTERFACE_HPP_INCLUDED
#define _STRUS_STORAGE_ALTER_META_DATA_TABLE_INTERFACE_HPP_INCLUDED
#include <string>
#include "strus/arithmeticVariant.hpp"

namespace strus
{

/// \class StorageAlterMetaDataTableInterface
/// \brief Object to declare changes in the meta data table structure of the storage
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
	virtual void commit()=0;

	/// \brief Rollback of the meta data table changes, no changes made
	virtual void rollback()=0;
};

}//namespace
#endif


