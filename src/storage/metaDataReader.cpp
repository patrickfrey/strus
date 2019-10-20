/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "metaDataReader.hpp"
#include "metaDataRestriction.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <string>

using namespace strus;

MetaDataReader::MetaDataReader(
		const strus::shared_ptr<MetaDataBlockCache>& cache_,
		ErrorBufferInterface* errorhnd_)
	:m_cache(cache_),m_docno(0),m_errorhnd(errorhnd_)
{
	m_description = &m_cache->descr();
	m_current = MetaDataRecord( &m_cache->descr(), 0);
}

Index MetaDataReader::elementHandle( const std::string& name) const
{
	try
	{
		return m_description->getHandle( name);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error meta data get element handle: %s"), *m_errorhnd, -1);
}

void MetaDataReader::skipDoc( const Index& docno)
{
	try
	{
		if (docno != m_docno)
		{
			m_current = m_cache->get( m_docno=docno);
		}
	}
	CATCH_ERROR_MAP( _TXT("error meta data skip document: %s"), *m_errorhnd);
}

NumericVariant MetaDataReader::getValue( const Index& elementHandle_) const
{
	if (!m_docno) m_errorhnd->report( ErrorCodeIncompleteInitialization, _TXT("metadata reader cursor not set to document number"));
	return m_current.getValue( m_description->get( elementHandle_));
}

const char* MetaDataReader::getType( const Index& elementHandle_) const
{
	try
	{
		return m_description->get( elementHandle_)->typeName();
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error meta data get element type: %s"), *m_errorhnd, 0);
}

const char* MetaDataReader::getName( const Index& elementHandle_) const
{
	try
	{
		return m_description->getName( elementHandle_);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error meta data get element name: %s"), *m_errorhnd, 0);
}

std::vector<std::string> MetaDataReader::getNames() const
{
	try
	{
		return m_description->columns();
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error meta data get element names: %s"), *m_errorhnd, std::vector<std::string>());
}

Index MetaDataReader::nofElements() const
{
	return (Index)m_description->nofElements();
}




