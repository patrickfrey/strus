/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
		:m_dbcursor(database_),m_hasInit(false),m_hasValue(false),m_errorhnd(errorhnd_)
	{}

	virtual ~ValueIterator(){}

	virtual void skip( const char* value, std::size_t size)
	{
		try
		{
			Index val;
			m_hasValue = m_dbcursor.skip( std::string( value, size), m_value, val);
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
				if (m_dbcursor.loadFirst( key, value))
				{
					rt.push_back( key);
					m_hasInit = true;
				}
			}
			while (rt.size() < maxNofElements && m_dbcursor.loadNext( key, value))
			{
				rt.push_back( key);
			}
			return rt;
		}
		CATCH_ERROR_MAP_RETURN( _TXT("error fetching values from storage value iterator: %s"), *m_errorhnd, std::vector<std::string>());
	}

private:
	typename DatabaseAdapter::Cursor m_dbcursor;
	bool m_hasInit;
	bool m_hasValue;
	std::string m_value;
	ErrorBufferInterface* m_errorhnd;
};

}//namespace
#endif


