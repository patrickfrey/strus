/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "metaDataRecord.hpp"
#include "private/internationalization.hpp"
#include "floatConversions.hpp"
#include "indexPacker.hpp"
#include <utility>
#include <vector>
#include <stdexcept>
#include <iostream>

using namespace strus;

#undef STRUS_LOWLEVEL_DEBUG

template <typename ValueType>
static inline void setValue_( const MetaDataDescription& descr, void* ptr, const MetaDataElement* elem, const ValueType& value)
{
	switch (elem->type())
	{
		case MetaDataElement::Int8:
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "write meta data element int8 at offset " << elem->ofs() << " address " << std::hex << (uintptr_t)((char*)ptr + elem->ofs()) << std::dec << std::endl;
#endif
			*(int8_t*)((char*)ptr + elem->ofs()) = (int8_t)value;
			break;
		case MetaDataElement::UInt8:
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "write meta data element uint8 at offset " << elem->ofs() << " address " << std::hex << (uintptr_t)((char*)ptr + elem->ofs()) << std::dec <<  std::endl;
#endif
			*(uint8_t*)((char*)ptr + elem->ofs()) = (uint8_t)value;
			break;
		case MetaDataElement::Int16:
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "write meta data element int16 at offset " << elem->ofs() << " address " << std::hex << (uintptr_t)((char*)ptr + elem->ofs()) << std::dec <<  std::endl;
#endif
			*(int16_t*)((char*)ptr + elem->ofs()) = (int16_t)value;
			break;
		case MetaDataElement::UInt16:
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "write meta data element uint16 at offset " << elem->ofs() << " address " << std::hex << (uintptr_t)((char*)ptr + elem->ofs()) << std::dec <<  std::endl;
#endif
			*(uint16_t*)((char*)ptr + elem->ofs()) = (uint16_t)value;
			break;
		case MetaDataElement::Int32:
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "write meta data element int32 at offset " << elem->ofs() << " address " << std::hex << (uintptr_t)((char*)ptr + elem->ofs()) << std::dec <<  std::endl;
#endif
			*(int32_t*)((char*)ptr + elem->ofs()) = (int32_t)value;
			break;
		case MetaDataElement::UInt32:
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "write meta data element uint32 at offset " << elem->ofs() << " address " << std::hex << (uintptr_t)((char*)ptr + elem->ofs()) << std::dec <<  std::endl;
#endif
			*(uint32_t*)((char*)ptr + elem->ofs()) = (uint32_t)value;
			break;
		case MetaDataElement::Float16:
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "write meta data element float16 at offset " << elem->ofs() << " address " << std::hex << (uintptr_t)((char*)ptr + elem->ofs()) << std::dec <<  std::endl;
#endif
			*(float16_t*)((char*)ptr + elem->ofs()) = floatSingleToHalfPrecision( (float)value);
			break;
		case MetaDataElement::Float32:
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "write meta data element float32 at offset " << elem->ofs() << " address " << std::hex << (uintptr_t)((char*)ptr + elem->ofs()) << std::dec <<  std::endl;
#endif
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
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "read meta data element int8 at offset " << elem->ofs() << " address " << std::hex << (uintptr_t)((char*)ptr + elem->ofs()) << std::dec <<  std::endl;
#endif
			return (ValueType)*(int8_t*)((const char*)ptr + elem->ofs());

		case MetaDataElement::UInt8:
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "read meta data element uint8 at offset " << elem->ofs() << " address " << std::hex << (uintptr_t)((char*)ptr + elem->ofs()) << std::dec <<  std::endl;
#endif
			return (ValueType)*(uint8_t*)((const char*)ptr + elem->ofs());

		case MetaDataElement::Int16:
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "read meta data element int16 at offset " << elem->ofs() << " address " << std::hex << (uintptr_t)((char*)ptr + elem->ofs()) << std::dec <<  std::endl;
#endif
			return (ValueType)*(int16_t*)((const char*)ptr + elem->ofs());

		case MetaDataElement::UInt16:
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "read meta data element uint16 at offset " << elem->ofs() << " address " << std::hex << (uintptr_t)((char*)ptr + elem->ofs()) << std::dec <<  std::endl;
#endif
			return (ValueType)*(uint16_t*)((const char*)ptr + elem->ofs());

		case MetaDataElement::Int32:
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "read meta data element int32 at offset " << elem->ofs() << " address " << std::hex << (uintptr_t)((char*)ptr + elem->ofs()) << std::dec <<  std::endl;
#endif
			return (ValueType)*(int32_t*)((const char*)ptr + elem->ofs());

		case MetaDataElement::UInt32:
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "read meta data element uint32 at offset " << elem->ofs() << " address " << std::hex << (uintptr_t)((char*)ptr + elem->ofs()) << std::dec <<  std::endl;
#endif
			return (ValueType)*(uint32_t*)((const char*)ptr + elem->ofs());

		case MetaDataElement::Float16:
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "read meta data element float16 at offset " << elem->ofs() << " address " << std::hex << (uintptr_t)((char*)ptr + elem->ofs()) << std::dec <<  std::endl;
#endif
			return (ValueType)floatHalfToSinglePrecision( *(float16_t*)((const char*)ptr + elem->ofs()));

		case MetaDataElement::Float32:
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "read meta data element float32 at offset " << elem->ofs() << " address " << std::hex << (uintptr_t)((char*)ptr + elem->ofs()) << std::dec <<  std::endl;
#endif
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

void MetaDataRecord::setValue( const MetaDataElement* elem, const NumericVariant& value_)
{
	switch (value_.type)
	{
		case NumericVariant::Null:   clearValue( elem); break;
		case NumericVariant::Int:    setValueInt( elem, value_); break;
		case NumericVariant::UInt:   setValueUInt( elem, value_); break;
		case NumericVariant::Float:  setValueFloat( elem, (double)value_); break;
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

NumericVariant MetaDataRecord::getValue( const MetaDataElement* elem) const
{
	switch (elem->type())
	{
		case MetaDataElement::Int8:
			return NumericVariant( (int)*(int8_t*)((const char*)m_ptr + elem->ofs()));

		case MetaDataElement::UInt8:
			return NumericVariant( (unsigned int)*(uint8_t*)((const char*)m_ptr + elem->ofs()));

		case MetaDataElement::Int16:
			return NumericVariant( (int)*(int16_t*)((const char*)m_ptr + elem->ofs()));

		case MetaDataElement::UInt16:
			return NumericVariant( (unsigned int)*(uint16_t*)((const char*)m_ptr + elem->ofs()));

		case MetaDataElement::Int32:
			return NumericVariant( (int)*(int32_t*)((const char*)m_ptr + elem->ofs()));

		case MetaDataElement::UInt32:
			return NumericVariant( (unsigned int)*(uint32_t*)((const char*)m_ptr + elem->ofs()));

		case MetaDataElement::Float16:
			return NumericVariant( floatHalfToSinglePrecision( *(float16_t*)((const char*)m_ptr + elem->ofs())));

		case MetaDataElement::Float32:
			return NumericVariant( *(float*)((const char*)m_ptr + elem->ofs()));
	}
	return NumericVariant();
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
						dr.setValueFloat( ti->dst, val);
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
			<< " '" << getValue( elem).tostring().c_str() << "'"
			<< std::endl;
	}
}


