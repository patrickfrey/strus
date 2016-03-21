/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for reading elements of the document meta data table in the storage
/// \file "metaDataReaderInterface.hpp"
#ifndef _STRUS_METADATA_READER_INTERFACE_HPP_INCLUDED
#define _STRUS_METADATA_READER_INTERFACE_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/arithmeticVariant.hpp"
#include <string>

namespace strus
{

/// \brief Forward declaration
class MetaDataRestrictionInterface;

/// \brief Interface for accessing meta data from a strus storage
class MetaDataReaderInterface
{
public:
	/// \brief Destructor
	virtual ~MetaDataReaderInterface(){}

	/// \brief Find out if there exists a meta data table element with the specified name
	/// \param[in] name_ name of the element to check
	/// \return return true, if the element exists
	virtual bool hasElement( const std::string& name_) const=0;

	/// \brief Get the handle for a table element addressed by name
	/// \param[in] name_ name of the element
	/// \return return the element handle
	/// \remark Element handles are numbered from 0 to N-1, where N is what nofElements() returns
	virtual Index elementHandle( const std::string& name_) const=0;

	/// \brief Get the number of columns in the meta data table
	/// \return return number of columns
	/// \remark Element handles are numbered from 0 to N-1, where N is what nofElements() returns
	virtual Index nofElements() const=0;

	/// \brief Skip to the document number equal to 'docno'
	/// \param[in] docno document number
	/// \remark an initialized record is created for the document if the meta data block for this document does not exist yet
	virtual void skipDoc( const Index& docno)=0;

	/// \brief Get the value a table element
	/// \param[in] elementHandle_ the handle for the element
	/// \return return the element value as atomic variant (type depending on internal representation)
	virtual ArithmeticVariant getValue( const Index& elementHandle_) const=0;

	/// \brief Get the type name of a table element (internal representation)
	/// \param[in] elementHandle_ the handle for the element
	/// \return return the table element type name string
	virtual const char* getType( const Index& elementHandle_) const=0;

	/// \brief Get the name of a table element
	/// \param[in] elementHandle_ the handle for the element
	/// \return return the table element name
	virtual const char* getName( const Index& elementHandle_) const=0;
};
}//namespace
#endif

