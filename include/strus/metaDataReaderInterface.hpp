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
#ifndef _STRUS_METADATA_READER_INTERFACE_HPP_INCLUDED
#define _STRUS_METADATA_READER_INTERFACE_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/arithmeticVariant.hpp"
#include <string>

namespace strus
{

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

