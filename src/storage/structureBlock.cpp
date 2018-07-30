/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "structureBlock.hpp"
#include "memBlock.hpp"
#include "indexPacker.hpp"
#include "private/internationalization.hpp"
#include <cstring>
#include <limits>
#include <iostream>

using namespace strus;

void StructureBlock::initFrame()
{
	if (size() < sizeof(BlockHeader))
	{
		m_docindexptr = 0;
		m_docindexptr = 0;
		m_memberptr = 0;
	}
	else
	{
		const BlockHeader* hdr = (const BlockHeader*)ptr();
		if (hdr->docindexidx < (int)sizeof(BlockHeader) || hdr->docindexidx > (int)DataBlock::size()
		||  hdr->headeridx < (int)sizeof(BlockHeader) || hdr->headeridx > (int)DataBlock::size()
		||  hdr->memberidx < (int)sizeof(BlockHeader) || hdr->memberidx > (int)DataBlock::size())
		{
			throw strus::runtime_error( _TXT( "data corruption in structure block"));
		}
		m_docindexptr = (const DocIndexNode*)data_at( hdr->docindexidx);
		m_headerptr = (const StructureDef*)data_at( hdr->headeridx);
		m_memberptr = (const StructureMember*)data_at( hdr->memberidx);
	}
}
