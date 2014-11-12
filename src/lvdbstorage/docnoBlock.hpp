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
#ifndef _STRUS_LVDB_DOCNO_BLOCK_HPP_INCLUDED
#define _STRUS_LVDB_DOCNO_BLOCK_HPP_INCLUDED
#include "strus/index.hpp"
#include "databaseKey.hpp"
#include "floatConversions.hpp"
#include <stdint.h>
#include <leveldb/db.h>

namespace strus {

class DocnoBlock
{
public:
	class Element
	{
	public:
		Element() :m_docno(0),m_ff(0),m_weight(0.0){}
		Element( Index docno_, unsigned int ff_, float weight_);
		Element( const Element& o);

		float weight() const;

		unsigned int ff() const
		{
			return m_ff;
		}

		Index docno() const
		{
			return m_docno;
		}

	private:
		enum {Max_ff=0xffFF};
		uint32_t m_docno;
		uint16_t m_ff;
		strus::float16_t m_weight;	///< IEEE 754 half-precision binary floating-point format: binary16
	};

public:
	DocnoBlock();
	DocnoBlock( const Element* ar_, std::size_t arsize_);
	DocnoBlock( const DocnoBlock& o);

	bool empty() const
	{
		return !m_ar;
	}

	std::size_t size() const
	{
		return m_arsize;
	}
	const Element* data() const
	{
		return m_ar;
	}

	const Element& back() const
	{
		return m_ar[ m_arsize-1];
	}

	const Element* find( const Index& docno_) const;
	const Element* upper_bound( const Index& docno_) const;

	void clear()
	{
		m_ar = 0;
		m_arsize = 0;
	}

	void init( const Element* ar_, std::size_t arsize_);

private:
	const Element* m_ar;
	std::size_t m_arsize;
};

}
#endif

