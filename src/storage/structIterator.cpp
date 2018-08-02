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

using namespace strus;

IndexRange StructIterator::skipPosSource( const Index& firstpos_)
{
	if (!docno()) return IndexRange(0,0);
	if (m_structureScanner.initialized())
	{
		const StructBlock::StructureDef* cur = m_structureScanner.current();
		if (cur->header_start <= firstpos_ && cur->header_end > firstpos_)
		{
			return IndexRange( cur->header_start, cur->header_end);
		}
	}
	else
	{
		m_structureScanner = currentBlock().structureScanner_at( currentBlockCursor());
		if (!m_structureScanner.initialized()) return IndexRange(0,0);
	}
	m_memberScanner.clear();
	return m_structureScanner.skip( firstpos_);
}

IndexRange StructIterator::skipPosSink( const Index& firstpos_)
{
	if (!docno()) return IndexRange(0,0);
	if (m_memberScanner.initialized())
	{
		const StructBlock::StructureMember* cur = m_memberScanner.current();
		if (cur->start <= firstpos_ && cur->end > firstpos_)
		{
			return IndexRange( cur->start, cur->end);
		}
	}
	else
	{
		if (!m_structureScanner.initialized()) return IndexRange(0,0);
		m_memberScanner = m_structureScanner.members();
		if (!m_memberScanner.initialized()) return IndexRange(0,0);
	}
	return m_memberScanner.skip( firstpos_);
}


