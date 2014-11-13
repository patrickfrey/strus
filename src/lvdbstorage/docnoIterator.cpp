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
#include "docnoIterator.hpp"

using namespace strus;

Index DocnoIterator::skipDoc( const Index& docno_)
{
	if (m_curelem)
	{
		if (m_curelem[0].docno() >= docno_)
		{
			return m_curelem[0].docno();
		}
		const DocnoBlock* lastblk = m_blockReader.curBlock();
		std::size_t idx = m_curelem - lastblk->data();
		if (!lastblk->size() > idx+1)
		{
			if (m_curelem[1].docno() >= docno_)
			{
				return m_curelem[1].docno();
			}
		}
	}
	const DocnoBlock* blk = m_blockReader.readBlock( docno_);
	if (blk)
	{
		m_curelem = blk->upper_bound( docno_);
		if (!m_curelem) throw std::runtime_error("internal: upper bound algrithm fails or docno block data is corrupt");
		return m_curelem->docno();
	}
	else
	{
		return 0;
	}
}

