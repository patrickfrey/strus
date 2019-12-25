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

strus::Index StructBlockBuilder::DocStructureMapIterator::skipDoc( strus::Index docno)
{
	if (m_aridx >= (int)m_ar.size()) return 0;
	int idx = 0;
	if (docno <= m_ar[ m_aridx].docno)
	{
		if (docno == m_ar[ m_aridx].docno)
		{
			if (!m_itr_defined)
			{
				m_itr_defined = true;
				m_itr = m_ar[ m_aridx].map.begin();
			}
			return docno;
		}
		idx = m_ar.upperbound( docno, 0, m_aridx+1);
	}
	else
	{
		idx = m_ar.upperbound( docno, m_aridx+1, m_ar.size());
	}
	if (true==(m_itr_defined = (idx >= 0)))
	{
		m_aridx = idx;
		m_itr = m_ar[ m_aridx].map.begin();
		return m_ar[ m_aridx].docno;
	}
	return 0;
}

bool StructBlockBuilder::DocStructureMapIterator::scanNext( strus::Index& docno_, strus::IndexRange& range_, int& relid)
{
	if (m_itr_defined)
	{
		range_ = m_itr->first;
		relid_ = m_itr->second;
		docno_ = m_ar[ m_aridx].docno;
		if (++m_itr == m_ar[ m_aridx].map.end())
		{
			do
			{
				if (m_aridx+1 == m_ar.size())
				{
					m_itr_defined = false;
					return false;
				}
				++m_aridx;
				m_itr = m_ar[ m_aridx].map.begin();
			}
			while (m_itr == m_ar[ m_aridx].map.end());
		}
		return true;
	}
	else
	{
		return false;
	}
}

void StructBlockBuilder::DocStructureMapIterator::clear()
{
	m_itr_defined = false;
	m_aridx = 0;
}

