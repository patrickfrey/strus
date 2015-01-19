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
#ifndef _STRUS_METADATA_READER_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_METADATA_READER_IMPLEMENTATION_HPP_INCLUDED
#include "strus/metaDataReaderInterface.hpp"
#include "metaDataBlockCache.hpp"
#include "metaDataRecord.hpp"
#include "metaDataDescription.hpp"
#include "metaDataElement.hpp"

namespace strus
{

/// \brief Implementation of the MetaDataInterface
class MetaDataReader
	:public MetaDataReaderInterface
{
public:
	MetaDataReader( MetaDataBlockCache* cache_,
			const MetaDataDescription* description_)
		:m_cache(cache_),m_description(description_),m_current(description_,0),m_docno(0){}

	virtual Index elementHandle( const char* name) const
	{
		return m_description->getHandle( name);
	}

	virtual bool hasElement( const char* name) const
	{
		return m_description->hasElement( name);
	}

	virtual void skipDoc( const Index& docno)
	{
		if (docno != m_docno)
		{
			m_current = m_cache->get( m_docno=docno);
		}
	}

	virtual ArithmeticVariant getValue( const Index& elementHandle_) const
	{
		return m_current.getValue( m_description->get( elementHandle_));
	}

	virtual const char* getType( const Index& elementHandle_) const
	{
		return m_description->get( elementHandle_)->typeName();
	}

	virtual const char* getName( const Index& elementHandle_) const
	{
		return m_description->getName( elementHandle_);
	}

	virtual Index nofElements() const
	{
		return (Index)m_description->nofElements();
	}

private:
	MetaDataBlockCache* m_cache;
	const MetaDataDescription* m_description;
	MetaDataRecord m_current;
	Index m_docno;
};
}//namespace
#endif
