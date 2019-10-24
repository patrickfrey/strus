/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_METADATA_ELEMENT_HPP_INCLUDED
#define _STRUS_STORAGE_METADATA_ELEMENT_HPP_INCLUDED
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

	bool operator==( const MetaDataElement& o) const	{return isequal(o);}
	bool operator!=( const MetaDataElement& o) const	{return !isequal(o);}
	bool isequal( const MetaDataElement& o) const		{return m_type == o.m_type && m_ofs == o.m_ofs;}

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

