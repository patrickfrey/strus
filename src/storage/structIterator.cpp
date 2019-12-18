/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "structIterator.hpp"
#include "storageClient.hpp"
#include "private/internationalization.hpp"
#include "strus/errorBufferInterface.hpp"

#define INTERFACE_NAME "struct iterator"

using namespace strus;

IndexRange StructIteratorImpl::skipPosSource( const Index& firstpos_)
{
	if (!docno()) return m_source = IndexRange(0,0);
	if (firstpos_ > (Index)std::numeric_limits<StructBlock::PositionType>::max()) return m_source = IndexRange(0,0);

	if (!m_structureIterator.initialized())
	{
		m_structureIterator = currentBlock().structureIterator( currentBlockCursor());
	}
	return m_source = m_structureIterator.skip( firstpos_);
}

IndexRange StructIteratorImpl::skipPosSink( const Index& firstpos_)
{
	if (!docno()) return m_sink = IndexRange(0,0);
	if (firstpos_ > (Index)std::numeric_limits<StructBlock::PositionType>::max()) return m_sink = IndexRange(0,0);

	if (!m_memberIterator.initialized())
	{
		if (!m_structureIterator.initialized())
		{
			m_structureIterator = currentBlock().structureIterator( currentBlockCursor());
		}
		m_memberIterator = m_structureIterator.memberIterator();
	}
	return m_sink = m_memberIterator.skip( firstpos_);
}


Index StructIterator::skipDoc( const Index& docno)
{
	try
	{
		return m_impl.skipDoc( docno);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in %s skip source position: %s"), INTERFACE_NAME, *m_errorhnd, 0);
}
IndexRange StructIterator::skipPosSource( const Index& firstpos)
{
	try
	{
		return m_impl.skipPosSource( firstpos);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in %s skip source position: %s"), INTERFACE_NAME, *m_errorhnd, IndexRange(0,0));
}
IndexRange StructIterator::skipPosSink( const Index& firstpos)
{
	try
	{
		return m_impl.skipPosSink( firstpos);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in %s skip sink position: %s"), INTERFACE_NAME, *m_errorhnd, IndexRange(0,0));
}
IndexRange StructIterator::source() const
{
	try
	{
		return m_impl.source();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in %s get current source: %s"), INTERFACE_NAME, *m_errorhnd, IndexRange(0,0));
}
IndexRange StructIterator::sink() const
{
	try
	{
		return m_impl.sink();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in %s get current sink: %s"), INTERFACE_NAME, *m_errorhnd, IndexRange(0,0));
}


