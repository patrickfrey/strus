/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#include "metaDataReader.hpp"
#include "metaDataRestriction.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <string>

using namespace strus;

MetaDataReader::MetaDataReader( MetaDataBlockCache* cache_,
		const MetaDataDescription* description_,
		ErrorBufferInterface* errorhnd_)
	:m_cache(cache_),m_description(description_),m_current(description_,0),m_docno(0),m_errorhnd(errorhnd_){}

Index MetaDataReader::elementHandle( const std::string& name) const
{
	try
	{
		return m_description->getHandle( name);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error meta data get element handle: %s"), *m_errorhnd, -1);
}

bool MetaDataReader::hasElement( const std::string& name) const
{
	return m_description->hasElement( name);
}

void MetaDataReader::skipDoc( const Index& docno)
{
	try
	{
		if (docno != m_docno)
		{
			m_current = m_cache->get( m_docno=docno);
		}
	}
	CATCH_ERROR_MAP( _TXT("error meta data skip document: %s"), *m_errorhnd);
}

ArithmeticVariant MetaDataReader::getValue( const Index& elementHandle_) const
{
	return m_current.getValue( m_description->get( elementHandle_));
}

const char* MetaDataReader::getType( const Index& elementHandle_) const
{
	try
	{
		return m_description->get( elementHandle_)->typeName();
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error meta data get element type: %s"), *m_errorhnd, 0);
}

const char* MetaDataReader::getName( const Index& elementHandle_) const
{
	try
	{
		return m_description->getName( elementHandle_);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error meta data get element name: %s"), *m_errorhnd, 0);
}

Index MetaDataReader::nofElements() const
{
	return (Index)m_description->nofElements();
}




