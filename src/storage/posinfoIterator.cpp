/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "posinfoIterator.hpp"
#include "storageClient.hpp"
#include "private/internationalization.hpp"

using namespace strus;

Index PosinfoIterator::skipPos( const Index& firstpos_)
{
	if (!docno()) return 0;
	if (!m_positionScanner.initialized())
	{
		m_positionScanner = currentBlock().positionScanner_at( currentBlockCursor());
		if (!m_positionScanner.initialized()) return 0;
	}
	Index rt = m_positionScanner.skip( firstpos_);
	if (rt && rt < firstpos_)
	{
		throw strus::runtime_error( "%s",  _TXT( "corrupt index (posinfo block)"));
	}
	return rt;
}

unsigned int PosinfoIterator::frequency() const
{
	if (!docno()) return 0;
	return currentBlock().frequency_at( currentBlockCursor());
}

Index PosinfoIterator::documentFrequency() const
{
	if (m_documentFrequency < 0)
	{
		m_documentFrequency = m_storage->documentFrequency( m_termtypeno, m_termvalueno);
	}
	return m_documentFrequency;
}

