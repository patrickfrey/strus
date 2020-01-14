/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "structIterator.hpp"
#include "storageClient.hpp"
#include "private/errorUtils.hpp"
#include "private/internationalization.hpp"
#include "strus/errorBufferInterface.hpp"

#define INTERFACE_NAME "struct iterator"

using namespace strus;

StructIterator::StructIterator( const StorageClient* storage_, const DatabaseClientInterface* database_, ErrorBufferInterface* errorhnd_)
	:m_storage(storage_)
	,m_database(database_)
	,m_dbadapter()
	,m_curblock()
	,m_docno(0)
	,m_errorhnd(errorhnd_){}

StructIterator::~StructIterator(){}

void StructIterator::skipDoc( const Index& docno_)
{
	try
	{
		if (docno_ <= 0) throw std::runtime_error(_TXT("called skipDoc with an invalid document number"));
		if (docno_ == m_docno) return;
		if (!m_dbadapter.get())
		{
			m_dbadapter.reset( new DatabaseAdapter_StructBlock::Reader( m_database));
		}
		if (m_dbadapter->load( docno_, m_curblock))
		{
			m_docno = docno_;
			m_levels = m_curblock.fieldarsize();
			int li = 0;
			for (; li < m_levels; ++li)
			{
				m_scanner[ li] = m_curblock.fieldscanner( li);
			}
			for (; li < StructBlock::NofFieldLevels; ++li)
			{
				m_scanner[ li] = StructBlock::FieldScanner();
			}
		}
		else
		{
			m_docno = 0;
			m_levels = 0;
			int li = 0;
			for (; li < StructBlock::NofFieldLevels; ++li)
			{
				m_scanner[ li] = StructBlock::FieldScanner();
			}
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error in %s skip doc: %s"), INTERFACE_NAME, *m_errorhnd);
}

int StructIterator::levels() const
{
	return m_levels;
}

Index StructIterator::docno() const
{
	return m_docno;
}

IndexRange StructIterator::skipPos( int level, const Index& firstpos)
{
	strus::IndexRange rt;
	try
	{
		if (level >= 0 && level < m_levels)
		{
			rt = m_scanner[ level].skip( firstpos);
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in %s skip pos: %s"), INTERFACE_NAME, *m_errorhnd, rt);
}

IndexRange StructIterator::field( int level) const
{
	strus::IndexRange rt;
	try
	{
		if (level >= 0 && level < m_levels)
		{
			rt = m_scanner[ level].current();
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in %s get current field: %s"), INTERFACE_NAME, *m_errorhnd, rt);
}

StructureLinkArray StructIterator::links( int level) const
{
	StructureLinkArray rt;
	try
	{
		if (level >= 0 && level < m_levels)
		{
			rt = m_scanner[ level].links();
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in %s get current links: %s"), INTERFACE_NAME, *m_errorhnd, rt);
}


