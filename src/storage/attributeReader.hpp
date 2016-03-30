/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_ATTRIBUTE_READER_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_ATTRIBUTE_READER_IMPLEMENTATION_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/attributeReaderInterface.hpp"
#include "storageClient.hpp"
#include <string>

namespace strus
{

/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Interface for accessing document attributes from a strus storage
class AttributeReader
	:public AttributeReaderInterface
{
public:
	AttributeReader( const StorageClient* storage_, const DatabaseClientInterface* database_, ErrorBufferInterface* errorhnd_)
		:m_storage(storage_),m_database(database_),m_docno(0),m_errorhnd(errorhnd_){}

	virtual Index elementHandle( const char* name) const;

	virtual void skipDoc( const Index& docno)
	{
		m_docno = docno;
	}

	virtual std::string getValue( const Index& elementHandle_) const;

	virtual std::vector<std::string> getAttributeNames() const;

private:
	const StorageClient* m_storage;				///< storage handle
	const DatabaseClientInterface* m_database;		///< database handle
	Index m_docno;						///< current document number
	ErrorBufferInterface* m_errorhnd;			///< error buffer for exception free interface
};

}//namespace
#endif

