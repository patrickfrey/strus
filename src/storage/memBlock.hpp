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
#ifndef _STRUS_MEM_BLOCK_HPP_INCLUDED
#define _STRUS_MEM_BLOCK_HPP_INCLUDED
#include <cstring>
#include <cstdlib>

namespace strus {

/// \class MemBlock
/// \brief Helper class for exception save memory block handling
class MemBlock
{
public:
	explicit MemBlock( std::size_t blksize)
	{
		m_ptr = std::calloc( blksize, 1);
		if (!m_ptr) throw std::bad_alloc();
	}

	~MemBlock()
	{
		std::free( m_ptr);
	}

	void* ptr() const		{return m_ptr;}

private:
	void* m_ptr;
};

}
#endif


