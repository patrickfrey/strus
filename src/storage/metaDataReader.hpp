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
#ifndef _STRUS_METADATA_READER_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_METADATA_READER_IMPLEMENTATION_HPP_INCLUDED
#include "strus/metaDataReaderInterface.hpp"
#include "metaDataBlockCache.hpp"
#include "metaDataRecord.hpp"
#include "metaDataDescription.hpp"
#include "metaDataElement.hpp"
#include <string>
/*[-]*/#include <iostream>

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Implementation of the MetaDataInterface
class MetaDataReader
	:public MetaDataReaderInterface
{
public:
	MetaDataReader( MetaDataBlockCache* cache_,
			const MetaDataDescription* description_,
			ErrorBufferInterface* errorhnd_);
	virtual ~MetaDataReader()
	{
		/*[-]*/std::cout << "HALLY GALLY MetaDataReader" << std::endl;
	}

	virtual Index elementHandle( const std::string& name) const;
	virtual bool hasElement( const std::string& name) const;
	virtual void skipDoc( const Index& docno);
	virtual ArithmeticVariant getValue( const Index& elementHandle_) const;
	virtual const char* getType( const Index& elementHandle_) const;
	virtual const char* getName( const Index& elementHandle_) const;
	virtual Index nofElements() const;

private:
	MetaDataBlockCache* m_cache;
	const MetaDataDescription* m_description;
	MetaDataRecord m_current;
	Index m_docno;						///< current document number
	ErrorBufferInterface* m_errorhnd;			///< error buffer for exception free interface
};
}//namespace
#endif
