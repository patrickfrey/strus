/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Structure for storing similarity relations
#include "simRelationMap.hpp"
#include "private/internationalization.hpp"
#include <limits>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <set>
#include <algorithm>

#undef STRUS_LOWLEVEL_DEBUG

using namespace strus;

void SimRelationMap::addRow( const SampleIndex& index, const std::vector<Element>& ar)
{
	std::size_t aridx = m_ar.size();
	if (m_rowdescrmap.find( index) != m_rowdescrmap.end()) throw strus::runtime_error(_TXT("sim relation map row defined twice"));
	m_rowdescrmap[ index] = RowDescr( aridx, ar.size());
	m_ar.insert( m_ar.end(), ar.begin(), ar.end());
	std::sort( m_ar.begin() + aridx, m_ar.begin() + aridx + ar.size());
}

std::string SimRelationMap::tostring() const
{
	std::ostringstream buf;
	RowDescrMap::const_iterator ri = m_rowdescrmap.begin(), re = m_rowdescrmap.end();
	for (; ri != re; ++ri)
	{
		const Element& elem = m_ar[ ri->second.aridx];
		buf << "(" << ri->first << "," << elem.index << ") = " << elem.simdist << std::endl;
	}
	return buf.str();
}

