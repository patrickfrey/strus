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
#include "metaDataRecord.hpp"
#include "strus/private/arithmeticVariantAsString.hpp"
#include "private/internationalization.hpp"
#include "floatConversions.hpp"
#include "indexPacker.hpp"
#include <utility>
#include <vector>
#include <stdexcept>

using namespace strus;

template <typename ValueType>
static inline void setValue_( const MetaDataDescription& descr, void* ptr, const MetaDataElement* elem, const ValueType& value)
{
	switch (elem->type())
	{
		case MetaDataElement::Int8:
			*(int8_t*)((char*)ptr + elem->ofs()) = (int8_t)value;
			break;
		case MetaDataElement::UInt8:
			*(uint8_t*)((char*)ptr + elem->ofs()) = (uint8_t)value;
			break;
		case MetaDataElement::Int16:
			*(int16_t*)((char*)ptr + elem->ofs()) = (int16_t)value;
			break;
		case MetaDataElement::UInt16:
			*(uint16_t*)((char*)ptr + elem->ofs()) = (uint16_t)value;
			break;
		case MetaDataElement::Int32:
			*(int32_t*)((char*)ptr + elem->ofs()) = (int32_t)value;
			break;
		case MetaDataElement::UInt32:
			*(uint32_t*)((char*)ptr + elem->ofs()) = (uint32_t)value;
			break;
		case MetaDataElement::Float16:
			*(float16_t*)((char*)ptr + elem->ofs()) = floatSingleToHalfPrecision( (float)value);
			break;
		case MetaDataElement::Float32:
			*(float*)((char*)ptr + elem->ofs()) = (float)value;
			break;
		default:
			throw strus::logic_error( _TXT( "unknown meta data type"));
	}
}

template <typename ValueType>
static inline ValueType getValue_( const MetaDataDescription& descr, const void* ptr, const MetaDataElement* elem)
{
	switch (elem->type())
	{
		case MetaDataElement::Int8:
			return (ValueType)*(int8_t*)((const char*)ptr + elem->ofs());

		case MetaDataElement::UInt8:
			return (ValueType)*(uint8_t*)((const char*)ptr + elem->ofs());

		case MetaDataElement::Int16:
			return (ValueType)*(int16_t*)((const char*)ptr + elem->ofs());

		case MetaDataElement::UInt16:
			return (ValueType)*(uint16_t*)((const char*)ptr + elem->ofs());

		case MetaDataElement::Int32:
			return (ValueType)*(int32_t*)((const char*)ptr + elem->ofs());

		case MetaDataElement::UInt32:
			return (ValueType)*(uint32_t*)((const char*)ptr + elem->ofs());

		case MetaDataElement::Float16:
			return (ValueType)floatHalfToSinglePrecision( *(float16_t*)((const char*)ptr + elem->ofs()));

		case MetaDataElement::Float32:
			return (ValueType)*(float*)((const char*)ptr + elem->ofs());
	}
	throw strus::logic_error( _TXT( "unknown meta data type"));
}

void MetaDataRecord::setValueInt( const MetaDataElement* elem, int32_t value_)
{
	setValue_( *m_descr, m_ptr, elem, value_);
}

void MetaDataRecord::setValueUInt( const MetaDataElement* elem, uint32_t value_)
{
	setValue_( *m_descr, m_ptr, elem, value_);
}

void MetaDataRecord::setValueFloat( const MetaDataElement* elem, float value_)
{
	setValue_( *m_descr, m_ptr, elem, value_);
}

void MetaDataRecord::setValue( const MetaDataElement* elem, const ArithmeticVariant& value_)
{
	switch (value_.type)
	{
		case ArithmeticVariant::Null:   clearValue( elem); break;
		case ArithmeticVariant::Int:    setValueInt( elem, value_); break;
		case ArithmeticVariant::UInt:   setValueUInt( elem, value_); break;
		case ArithmeticVariant::Float:  setValueFloat( elem, value_); break;
	}
}

int MetaDataRecord::getValueInt( const MetaDataElement* elem) const
{
	return getValue_<int>( *m_descr, m_ptr, elem);
}

unsigned int MetaDataRecord::getValueUInt( const MetaDataElement* elem) const
{
	return getValue_<unsigned int>( *m_descr, m_ptr, elem);
}

float MetaDataRecord::getValueFloat( const MetaDataElement* elem) const
{
	return getValue_<float>( *m_descr, m_ptr, elem);
}

ArithmeticVariant MetaDataRecord::getValue( const MetaDataElement* elem) const
{
	switch (elem->type())
	{
		case MetaDataElement::Int8:
			return ArithmeticVariant( *(int8_t*)((const char*)m_ptr + elem->ofs()));

		case MetaDataElement::UInt8:
			return ArithmeticVariant( *(uint8_t*)((const char*)m_ptr + elem->ofs()));

		case MetaDataElement::Int16:
			return ArithmeticVariant( *(int16_t*)((const char*)m_ptr + elem->ofs()));

		case MetaDataElement::UInt16:
			return ArithmeticVariant( *(uint16_t*)((const char*)m_ptr + elem->ofs()));

		case MetaDataElement::Int32:
			return ArithmeticVariant( *(int32_t*)((const char*)m_ptr + elem->ofs()));

		case MetaDataElement::UInt32:
			return ArithmeticVariant( *(uint32_t*)((const char*)m_ptr + elem->ofs()));

		case MetaDataElement::Float16:
			return ArithmeticVariant( floatHalfToSinglePrecision( *(float16_t*)((const char*)m_ptr + elem->ofs())));

		case MetaDataElement::Float32:
			return ArithmeticVariant( *(float*)((const char*)m_ptr + elem->ofs()));
	}
	return ArithmeticVariant();
}

void MetaDataRecord::clearValue( const MetaDataElement* elem)
{
	std::memset( (char*)m_ptr + elem->ofs(), 0, elem->size());
}

void MetaDataRecord::translateBlock(
		const MetaDataDescription::TranslationMap& translationMap,
		const MetaDataDescription& dest,
		void* blkdest,
		const MetaDataDescription& src,
		const void* blksrc,
		std::size_t nofelem)
{
	std::memset( blkdest, 0, nofelem * dest.bytesize());
	MetaDataDescription::TranslationMap::const_iterator
		ti = translationMap.begin(),
		te = translationMap.end();
	for (; ti != te; ++ti)
	{
		if (ti->dst->type() == ti->src->type())
		{
			std::size_t elemsize = ti->dst->size();
			std::size_t destofs = ti->dst->ofs();
			std::size_t srcofs = ti->src->ofs();

			char* destrec = (char*)blkdest;
			char const* srcrec = (const char*)blksrc;

			for (std::size_t ii=0; ii<nofelem;
				++ii,destrec+=dest.bytesize(),srcrec+=src.bytesize())
			{
				std::memcpy( destrec+destofs, srcrec+srcofs, elemsize);
			}
		}
		else
		{
			char* destrec = (char*)blkdest;
			char const* srcrec = (const char*)blksrc;

			for (std::size_t ii=0; ii<nofelem;
				++ii,destrec+=dest.bytesize(),srcrec+=src.bytesize())
			{
				MetaDataRecord sr( &src, const_cast<char*>(srcrec));
				MetaDataRecord dr( &src, destrec);

				switch (ti->src->type())
				{
					case MetaDataElement::Int8:
					case MetaDataElement::UInt8:
					case MetaDataElement::Int16:
					{
						int val = sr.getValueInt( ti->src);
						dr.setValueInt( ti->dst, val);
						break;
					}
					case MetaDataElement::UInt16:
					case MetaDataElement::Int32:
					case MetaDataElement::UInt32:
					{
						unsigned int val = sr.getValueUInt( ti->src);
						dr.setValueUInt( ti->dst, val);
						break;
					}
					case MetaDataElement::Float16:
					case MetaDataElement::Float32:
					{
						float val = sr.getValueFloat( ti->src);
						dr.setValueUInt( ti->dst, val);
						break;
					}
				}
			}
		}
	}
}

void MetaDataRecord::print( std::ostream& out) const
{
	std::size_t ei=0, ee=m_descr->nofElements();
	for (; ei != ee; ++ei)
	{
		const MetaDataElement* elem = m_descr->get( ei);
		out << "[" << ei << "] " 
			<< m_descr->getName( ei) << " "
			<< elem->typeName()
			<< " '" << getValue( elem) << "'"
			<< std::endl;
	}
}


