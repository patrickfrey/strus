/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_METADATA_RECORD_HPP_INCLUDED
#define _STRUS_STORAGE_METADATA_RECORD_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/numericVariant.hpp"
#include "metaDataDescription.hpp"
#include "metaDataElement.hpp"
#include "strus/base/stdint.h"
#include <utility>
#include <cstring>
#include <iostream>

namespace strus {

class MetaDataRecord
{
public:
	MetaDataRecord( const MetaDataDescription* description_, void* ptr_)
		:m_descr(description_),m_ptr(ptr_){}

	void setValueInt( const MetaDataElement* elem, int64_t value_);
	void setValueUInt( const MetaDataElement* elem, uint64_t value_);
	void setValueFloat( const MetaDataElement* elem, double value_);
	void setValue( const MetaDataElement* elem, const NumericVariant& value_);

	int getValueInt( const MetaDataElement* elem) const;
	unsigned int getValueUInt( const MetaDataElement* elem) const;
	float getValueFloat( const MetaDataElement* elem) const;
	NumericVariant getValue( const MetaDataElement* elem) const;

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
			const MetaDataDescription::TranslationMap& translationMap,
			const MetaDataDescription& dest,
			void* blkdest,
			const MetaDataDescription& src,
			const void* blksrc,
			std::size_t nofelem);

	void print( std::ostream& out) const;

private:
	const MetaDataDescription* m_descr;
	void* m_ptr;
};

}
#endif

