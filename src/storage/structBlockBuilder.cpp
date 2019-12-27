/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "structBlockBuilder.hpp"
#include "private/internationalization.hpp"
#include <cstring>
#include <limits>

using namespace strus;

strus::Index StructBlockBuilder::DocStructureMapScanner::skipDoc( strus::Index docno)
{
	if (m_aridx >= (int)m_ar.size()) return 0;
	int idx = 0;
	if (docno <= m_ar[ m_aridx].docno)
	{
		if (docno == m_ar[ m_aridx].docno)
		{
			return docno;
		}
		idx = m_ar.upperbound( docno, 0, m_aridx+1);
	}
	else
	{
		idx = m_ar.upperbound( docno, m_aridx+1, m_ar.size());
	}
	return (idx >= 0) ? m_ar[ m_aridx = idx].docno : 0;
}


