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
/// \brief Structure for passing read/write options to the the key/value store database
/// \file "databaseOptions.hpp"
#ifndef _STRUS_DATABASE_OPTIONS_HPP_INCLUDED
#define _STRUS_DATABASE_OPTIONS_HPP_INCLUDED

namespace strus
{

/// \brief Structure for passing some options to the strus key value storage database
class DatabaseOptions
{
public:
	/// \brief Default constructor
	DatabaseOptions()
		:m_opt(0){}
	/// \brief Copy constructor
	DatabaseOptions( const DatabaseOptions& o)
		:m_opt(o.m_opt){}
	/// \brief Constructor
	DatabaseOptions( unsigned int opt_)
		:m_opt(opt_){}

	/// \brief Enable caching of visited key/value elements or blocks
	DatabaseOptions& useCache( bool yes=true)
	{
		if (yes) m_opt |= UseCache; else m_opt &= ~UseCache;
		return *this;
	}

	/// \brief Test flag for caching the values read in an LRU cache
	bool useCacheEnabled() const
	{
		return m_opt & UseCache;
	}

	/// \brief Get the options transacription as integer
	unsigned int opt() const
	{
		return m_opt;
	}

private:
	unsigned int m_opt;
	enum {UseCache=1};
};

}//namespace
#endif

