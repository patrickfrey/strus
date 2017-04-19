/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_METADATA_READER_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_METADATA_READER_IMPLEMENTATION_HPP_INCLUDED
#include "strus/metaDataReaderInterface.hpp"
#include "strus/numericVariant.hpp"
#include "metaDataBlockCache.hpp"
#include "metaDataRecord.hpp"
#include "metaDataDescription.hpp"
#include "metaDataElement.hpp"
#include <string>

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Implementation of the MetaDataInterface
class MetaDataReader
	:public MetaDataReaderInterface
{
public:
	MetaDataReader( MetaDataBlockCache* cache_,
			const MetaDataDescription* description_,
			ErrorBufferInterface* errorhnd_);
	virtual ~MetaDataReader(){}

	virtual Index elementHandle( const std::string& name) const;
	virtual void skipDoc( const Index& docno);
	virtual NumericVariant getValue( const Index& elementHandle_) const;
	virtual const char* getType( const Index& elementHandle_) const;
	virtual const char* getName( const Index& elementHandle_) const;
	virtual Index nofElements() const;

private:
	MetaDataBlockCache* m_cache;
	const MetaDataDescription* m_description;
	MetaDataRecord m_current;
	Index m_docno;						///< current document number
	ErrorBufferInterface* m_errorhnd;			///< error buffer for exception free interface
};
}//namespace
#endif
