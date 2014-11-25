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
#ifndef _STRUS_LVDB_METADATA_RECORD_HPP_INCLUDED
#define _STRUS_LVDB_METADATA_RECORD_HPP_INCLUDED
#include "strus/index.hpp"
#include "metaDataDescription.hpp"
#include "metaDataElement.hpp"
#include "variant.hpp"
#include <utility>
#include <cstring>

namespace strus {

class MetaDataRecord
{
public:
	MetaDataRecord( const MetaDataDescription* description_, void* ptr_)
		:m_descr(description_),m_ptr(ptr_){}

	void setValueInt( const MetaDataElement* elem, int32_t value_);
	void setValueUInt( const MetaDataElement* elem, uint32_t value_);
	void setValueFloat( const MetaDataElement* elem, float value_);
	void setValue( const MetaDataElement* elem, const Variant& value_);

	int getValueInt( const MetaDataElement* elem) const;
	unsigned int getValueUInt( const MetaDataElement* elem) const;
	float getValueFloat( const MetaDataElement* elem) const;
	Variant getValue( const MetaDataElement* elem) const;

	void clearValue( const MetaDataElement* elem);

	void clear()
	{
		std::memset( m_ptr, 0, m_descr->bytesize());
	}

	void copy( const void* optr)
	{
		std::memcpy( m_ptr, optr, m_descr->bytesize());
	}

	static void translateBlock(
			const MetaDataDescription& dest,
			void* blkdest,
			const MetaDataDescription& src,
			const void* blksrc,
			std::size_t nofelem);

private:
	const MetaDataDescription* m_descr;
	void* m_ptr;
};

}
#endif

