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
#ifndef _STRUS_LVDB_STORAGE_CONFIG_HPP_INCLUDED
#define _STRUS_LVDB_STORAGE_CONFIG_HPP_INCLUDED
#include <string>

namespace strus {

class StorageConfig
{
public:
	explicit StorageConfig( const char* source);

	const std::string& path() const
	{
		return m_path;
	}

	/// \brief File with line separated list of terms to cache (insert or query)
	const std::string& termkeys() const
	{
		return m_termkeys;
	}

	bool acl() const
	{
		return m_acl;
	}

	const std::string& metadata() const
	{
		return m_metadata;
	}

	unsigned int cachesize_kb() const
	{
		return m_cachesize_kb;
	}

	static const char* getDescription();

private:
	std::string m_path;
	std::string m_metadata;
	std::string m_termkeys;
	bool m_acl;
	unsigned int m_cachesize_kb;
};
}//namespace
#endif



