/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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

