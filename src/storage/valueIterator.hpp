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
/// \brief Implementation of interface for introspection of string values stored in the storage
/// \file "valueIterator.hpp"
#ifndef _STRUS_VALUE_ITERATOR_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_VALUE_ITERATOR_IMPLEMENTATION_HPP_INCLUDED
#include "strus/valueIteratorInterface.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/databaseCursorInterface.hpp"
#include "strus/valueIteratorInterface.hpp"
#include "databaseAdapter.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"

namespace strus
{

template <class DatabaseAdapter>
class ValueIterator
	:public ValueIteratorInterface
{
public:
	ValueIterator( const DatabaseClientInterface* database_, ErrorBufferInterface* errorhnd_)
		:dbcursor(database_),m_hasInit(false),m_hasValue(false),m_errorhnd(errorhnd_)
	{}

	virtual ~ValueIterator(){}

	virtual void skip( const char* value, std::size_t size)
	{
		try
		{
			Index val;
			m_hasValue = dbcursor.skip( std::string( value, size), m_value, val);
			if (m_hasValue)
			{
				m_hasInit = true;
			}
		}
		CATCH_ERROR_MAP( _TXT( "error in skip to key of storage value iterator: %s"), *m_errorhnd);
	}

	virtual std::vector<std::string> fetchValues( std::size_t maxNofElements)
	{
		try
		{
			std::vector<std::string> rt;
			std::string key;
			Index value;
			if (m_hasValue)
			{
				rt.push_back( m_value);
				m_value.clear();
				m_hasValue = false;
			}
			if (!m_hasInit)
			{
				if (dbcursor.loadFirst( key, value))
				{
					rt.push_back( key);
					m_hasInit = true;
				}
			}
			while (rt.size() < maxNofElements && dbcursor.loadNext( key, value))
			{
				rt.push_back( key);
			}
			return rt;
		}
		CATCH_ERROR_MAP_RETURN( _TXT("error fetching values from storage value iterator: %s"), *m_errorhnd, std::vector<std::string>());
	}

private:
	typename DatabaseAdapter::Cursor dbcursor;
	bool m_hasInit;
	bool m_hasValue;
	std::string m_value;
	ErrorBufferInterface* m_errorhnd;
};

}//namespace
#endif


