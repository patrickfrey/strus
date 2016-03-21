/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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


