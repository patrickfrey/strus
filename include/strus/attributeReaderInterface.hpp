/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface (readonly) for accessing document attributes
/// \file "attributeReaderInterface.hpp"
#ifndef _STRUS_ATTRIBUTE_READER_INTERFACE_HPP_INCLUDED
#define _STRUS_ATTRIBUTE_READER_INTERFACE_HPP_INCLUDED
#include "strus/storage/index.hpp"
#include <string>
#include <vector>

namespace strus
{

/// \brief Interface for accessing document attributes from a strus storage
class AttributeReaderInterface
{
public:
	/// \brief Destructor
	virtual ~AttributeReaderInterface(){}

	/// \brief Get the handle of an element addressed by its name
	/// \param[in] name name of the element
	/// \return the element handle as number (count starting with 1) or 0 on error
	virtual Index elementHandle( const std::string& name) const=0;

	/// \brief Move the attribute reader context to a specific document
	/// \param[in] docno the number of the document to move the context to
	virtual void skipDoc( const Index& docno)=0;

	/// \brief Get the value of the attribute of the document in the current readed context -- defined with skipDoc(const Index&)
	/// \param[in] elementHandle_ the handle of the element to retrieve
	/// \return the value of the element as string or an empty string if undefined
	virtual std::string getValue( const Index& elementHandle_) const=0;

	/// \brief Get the names of all attributes defined in the storage
	/// \return list of attribute names
	/// \note Returns the names of all attributes defined for any document, independent of the current document.
	virtual std::vector<std::string> getNames() const=0;
};
}//namespace
#endif

