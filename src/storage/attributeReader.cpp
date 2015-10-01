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
		Index rt = m_storage->getAttributeno( name);
		if (!rt)
		{
			m_errorhnd->report( _TXT( "attribute with name '%s' is not defined"), name);
		}
		return rt;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error retrieving storage attribute element handle: %s"), *m_errorhnd, 0);
}

std::string AttributeReader::getValue( const Index& elementHandle_) const
{
	try
	{
		std::string rt;
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

std::vector<std::string> AttributeReader::getAttributeNames() const
{
	try
	{
		return m_storage->getAttributeNames();
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error reading storage attribute names: %s"), *m_errorhnd, std::vector<std::string>());
}

