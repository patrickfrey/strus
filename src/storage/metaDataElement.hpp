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
#ifndef _STRUS_LVDB_METADATA_ELEMENT_HPP_INCLUDED
#define _STRUS_LVDB_METADATA_ELEMENT_HPP_INCLUDED
#include "strus/index.hpp"
#include <utility>
#include <string>

namespace strus {

/// \brief Forward declaration
class MetaDataDescription;

class MetaDataElement
{
public:
	enum Type {Int8,UInt8,Int16,UInt16,Int32,UInt32,Float16,Float32};
	enum {NofTypes=Float32+1};

public:
	MetaDataElement( Type type_, std::size_t ofs_)
		:m_type(type_),m_ofs((unsigned char)ofs_){}

	MetaDataElement( const MetaDataElement& o)
		:m_type(o.m_type),m_ofs(o.m_ofs){}


	static unsigned int size( Type t)
	{
		static std::size_t ar[] = {1,1,2,2,4,4,2,4};
		return ar[t];
	}
	unsigned int size() const
	{
		return size(m_type);
	}
	static const char* typeName( Type t)
	{
		static const char* ar[] = {"Int8","UInt8","Int16","UInt16","Int32","UInt32","Float16","Float32"};
		return ar[t];
	}
	static Type typeFromName( const char* namestr);

	const char* typeName() const
	{
		return typeName(m_type);
	}
	unsigned int ofs() const
	{
		return m_ofs;
	}

	Type type() const			{return m_type;}

private:
	friend class MetaDataDescription;
	Type m_type;
	unsigned char m_ofs;
};

}//namespace
#endif

