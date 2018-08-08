/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "attributeReader.hpp"
#include "databaseAdapter.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <stdexcept>

using namespace strus;

Index AttributeReader::elementHandle( const char* name) const
{
	try
	{
		return m_storage->getAttributeno( name);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error retrieving storage attribute element handle: %s"), *m_errorhnd, 0);
}

std::string AttributeReader::getValue( const Index& elementHandle_) const
{
	try
	{
		std::string rt;
		if (!m_docno)
		{
			m_errorhnd->report( ErrorCodeIncompleteInitialization, _TXT("attribute reader cursor not set to document number"));
			return std::string();
		}
		if (DatabaseAdapter_DocAttribute::load( m_database, m_docno, elementHandle_, rt))
		{
			return rt;
		}
		else
		{
			return std::string();
		}
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error reading storage attribute value: %s"), *m_errorhnd, std::string());
}

std::vector<std::string> AttributeReader::getNames() const
{
	try
	{
		return m_storage->getAttributeNames();
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error reading storage attribute names: %s"), *m_errorhnd, std::vector<std::string>());
}

