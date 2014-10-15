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
#include "tupleGenerator.hpp"

using namespace strus;

TupleGenerator::TupleGenerator( Mode mode_)
	:m_empty(false),m_mode(mode_){}

void TupleGenerator::defineColumn( std::size_t max_)
{
	m_columns.push_back( Column( 0, max_));
	if (m_mode == Ascending || m_mode == Permutation)
	{
		std::vector<Column>::iterator ci = m_columns.begin(), ce = m_columns.end();
		std::size_t idx = m_columns.size()-1;
		for (; ci != ce; ++ci,--idx)
		{
			ci->value = idx;
			if (ci->value > ci->maximum)
			{
				m_empty = true;
			}
		}
		if (m_mode == Permutation)
		{
			while (m_columns.size() > m_occupied.size())
			{
				m_occupied.push_back( true);
			}
		}
	}
}

bool TupleGenerator::next()
{
	if (m_empty || !m_columns.size()) return false;
	if (m_mode == Sequence)
	{
		for (std::size_t ii=0, nn=m_columns.size(); ii<nn; ++ii)
		{
			if (m_columns[ ii].value >= m_columns[ii].maximum)
			{
				return false;
			}
			++m_columns[ ii].value;
		}
		return true;
	}
	else
	{
		std::size_t colidx = m_columns.size()-1;
	
		bool carry = incrementIndex( m_columns[ colidx].value, m_columns[ colidx].maximum);
		if (carry)
		{
			// Forward carry of increment:
			for (; colidx > 0; --colidx)
			{
				bool carry = incrementIndex( m_columns[ colidx-1].value, m_columns[ colidx-1].maximum);
				if (!carry)
				{
					break;
				}
			}
			if (colidx == 0)
			{
				// ... we have all rows
				m_empty = true;
				return false;
			}
			if (m_mode == Ascending)
			{
				// ... make column values ascending
				for (colidx=1; colidx<m_columns.size(); ++colidx)
				{
					if (m_columns[ colidx].value <= m_columns[ colidx-1].value)
					{
						if (m_columns[ colidx-1].value+1 > m_columns[ colidx].maximum)
						{
							// ... no row with ascending columns possible anymore
							m_empty = true;
							return false;
						}
						m_columns[ colidx].value = m_columns[ colidx-1].value+1;
					}
				}
			}
		}
	}
	return true;
}

bool TupleGenerator::incrementIndex( std::size_t& idx, const std::size_t& maximum)
{
	if (m_mode == Permutation)
	{
		m_occupied[ idx] = false;
		for (++idx; m_occupied[ idx] && idx < maximum; ++idx){}
		if (idx >= maximum)
		{
			for (idx=0; m_occupied[ idx] && idx < maximum; ++idx){}
			m_occupied[ idx] = true;
			return true;
		}
		else
		{
			m_occupied[ idx] = true;
			return false;
		}
	}
	else
	{
		++idx;
		if (idx >= maximum)
		{
			idx = 0;
			return true;
		}
		else
		{
			return false;
		}
	}
}

