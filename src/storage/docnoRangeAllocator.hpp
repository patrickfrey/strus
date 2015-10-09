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
#ifndef _STRUS_DOCNO_RANGE_ALLOCATOR_HPP_INCLUDED
#define _STRUS_DOCNO_RANGE_ALLOCATOR_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/docnoRangeAllocatorInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "storageClient.hpp"

namespace strus {

class DocnoRangeAllocator
	:public DocnoRangeAllocatorInterface
{
public:
	DocnoRangeAllocator( StorageClient* storage_, ErrorBufferInterface* errorhnd_)
		:m_storage(storage_),m_errorhnd(errorhnd_){}

	virtual Index allocDocnoRange( const Index& size)
	{
		try
		{
			return m_storage->allocDocnoRange( size);
		}
		CATCH_ERROR_MAP_RETURN( _TXT("error allocating document number range: %s"), *m_errorhnd, 0);
	}

	virtual bool deallocDocnoRange( const Index& docno, const Index& size)
	{
		try
		{
			return m_storage->deallocDocnoRange( docno, size);
		}
		CATCH_ERROR_MAP_RETURN( _TXT("error deallocating document number range: %s"), *m_errorhnd, false);
	}

private:
	StorageClient* m_storage;
	ErrorBufferInterface* m_errorhnd;		///< error buffer for exception free interface
};

}//namespace
#endif

