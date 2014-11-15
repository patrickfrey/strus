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
#include "posinfoBlock.hpp"
#include <cstring>

using namespace strus;

Index PosinfoBlock::Element::skipPos( const Index& pos_)
{
	if (m_posno >= firstpos_)
	{
		if (m_posno == firstpos_ && firstpos_ != 0)
		{
			return m_posno;
		}
		m_posno = 0;
		m_itr = m_ptr;
	}
	unsigned int ofs = (m_end - m_itr) >> 1;
	if (ofs > firstpos_ - m_posno)
	{
		ofs = (firstpos_ - m_posno) >> 4;
	}
	while (ofs >= 6)
	{
		const char* skipitr = strus::nextPackedIndexPos( m_itr, m_itr + ofs, m_end);
		if (skipitr != m_end)
		{
			Index nextpos = unpackIndex( skipitr, m_end);
			if (nextpos <= firstpos_)
			{
				m_posno = nextpos;
				m_itr = skipitr;
				if (nextpos == firstpos_)
				{
					return m_posno;
				}
				else
				{
					ofs = (m_end - m_itr) >> 1;
					if (ofs > firstpos_ - m_posno)
					{
						ofs = (firstpos_ - m_posno) >> 4;
					}
					continue;
				}
			}
			else
			{
				ofs /= 2;
			}
		}
	}
	while (m_itr < m_posend && (firstpos_ > m_posno || !m_posno))
	{
		// Get the next position:
		m_posno = unpackIndex( m_itr, m_end);
	}
	if (firstpos_ > m_posno)
	{
		return 0;
	}
	return m_posno;
}

bool PosinfoBlock::loadElement()
{
	if (m_blkitr == m_blkptr || *(m_blkitr-1) == 0xFF)
	{
		char const* end = std::memchr( m_blkitr, 0xFF, m_blkend-m_blkitr);
		if (!end) end = m_blkend;

		Index reldocno = unpackIndex( m_blkitr, end);
		m_elem.init( docno_ - reldocno, m_blkitr, end-m_blkitr);
	}
	else
	{
		throw std::logic_error( "illegal block element reference (PosinfoBlock::loadElement())");
	}
}

const Element* PosinfoBlock::upper_bound( const Index& docno_) const
{
	if (m_elem.empty())
	{
		char const* last = m_blkend;
		char const* first = m_blkptr;
		char const* mid = m_blkptr + ((m_blkend - m_blkptr) >> 4);

		while (first+8 < last)
		{
			mid = std::memrchr( mid, 0xFF, mid-first);
			if (mid)
			{
				++mid;
			}
			else
			{
				mid = first;
			}
			m_blkitr = mid;
			loadElement();

			if (m_elem.docno() > docno_)
			{
				last = m_elem.end();
			}
			else if (m_elem.docno() < docno_)
			{
				first = m_elem.end();
			}
			else
			{
				return docno_;
			}
		}
		while (first < last)
		{
			m_blkitr = first;
			loadElement();
			if (m_elem.docno() >= docno_)
			{
				return m_elem.docno();
			}
		}
		return 0;
	}
}

const Element* PosinfoBlock::find( const Index& docno_) const
{
	const Element* rt = upper_bound( docno_);
	return (rt && rt->docno() == docno_)?rt:0;
}


