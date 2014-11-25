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
#ifndef _STRUS_STORAGE_INSERTER_INTERFACE_HPP_INCLUDED
#define _STRUS_STORAGE_INSERTER_INTERFACE_HPP_INCLUDED
#include <string>
#include "strus/index.hpp"
#include "strus/arithmeticVariant.hpp"

namespace strus
{

/// \class StorageInserterInterface
/// \brief Object to declare all items for one insert/update of a document in the storage
class StorageInserterInterface
{
public:
	/// \brief Destructor that is the doing the rollback too, if done() was not called before
	virtual ~StorageInserterInterface(){}

	/// \brief Add one occurrence of a term, throws on std::bad_alloc
	/// \param[in] type_ type name of the term
	/// \param[in] value_ value string of the term
	/// \param[in] position_ position of the term in the document
	/// \param[in] weight_ additional weight of the term at this position in the document
	virtual void addTermOccurrence(
			const std::string& type_,
			const std::string& value_,
			const Index& position_,
			float weight_)=0;

	/// \brief Define a numeric attribute for the document with fast access for query evaluation
	/// \note Meta data are used for query restrictions and for document weights in query result ranking
	/// \remark Document meta data have to be declared in advance when creating the storage or with an alter metadata table command when no clients are running on this storage.
	/// \param[in] name_ name of the existing meta data element
	/// \param[in] value_ value of the element
	virtual void setMetaData(
			const std::string& name_,
			const ArithmeticVariant& value_)=0;

	/// \brief Define a string attribute for the document
	/// \param[in] name_ name of the attribute
	/// \param[in] value_ value of the document attribute
	virtual void setAttribute(
			const std::string& name_,
			const std::string& value_)=0;

	/// \brief Write the contents defined to the storage
	virtual void done()=0;
};

}//namespace
#endif


