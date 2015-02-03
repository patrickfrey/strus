/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#include "postingIterator.hpp"
#include "storage.hpp"
#include "indexPacker.hpp"
#include <string>
#include <vector>
#include <cstring>
#include <sstream>
#include <iostream>

using namespace strus;

#undef STRUS_LOWLEVEL_DEBUG
#undef STRUS_LOWLEVEL_CHECK

#ifdef STRUS_LOWLEVEL_DEBUG
PostingIterator::PostingIterator( DatabaseInterface* database_, Index termtypeno, Index termvalueno, const char* termstr)
#else
PostingIterator::PostingIterator( const Storage* storage_, DatabaseInterface* database_, Index termtypeno, Index termvalueno, const char*)
#endif
	:m_docnoIterator(database_, DatabaseKey::DocListBlockPrefix, BlockKey( termtypeno, termvalueno))
	,m_posinfoIterator(storage_,database_, termtypeno, termvalueno)
	,m_docno(0)
{
	m_featureid.reserve( 16);
#ifdef STRUS_LOWLEVEL_DEBUG
	m_featureid.append( termstr);
	m_featureid.push_back(':');
	m_featureid.push_back( (char)(termtypeno/10) + '0');
	m_featureid.push_back( (char)(termtypeno%10) + '0');
#else
	packIndex( m_featureid, termtypeno);
	packIndex( m_featureid, termvalueno);
#endif
}

Index PostingIterator::skipDoc( const Index& docno_)
{
	if (m_docno && m_docno == docno_) return m_docno;

	if (m_posinfoIterator.isCloseCandidate( docno_))
	{
		m_docno = m_posinfoIterator.skipDoc( docno_);
#ifdef STRUS_LOWLEVEL_CHECK
		if (m_docno != m_docnoIterator.skip( m_docno))
		{
			throw std::runtime_error( "corrupt index -- without posinfo");
		}
#endif
	}
	else
	{
		m_docno = m_docnoIterator.skip( docno_);
	}
	return m_docno;
}

Index PostingIterator::skipPos( const Index& firstpos_)
{
	if (m_docno != m_posinfoIterator.skipDoc( m_docno))
	{
		throw std::runtime_error( "corrupt index -- document not in posinfo index");
	}
	return m_posinfoIterator.skipPos( firstpos_);
}

unsigned int PostingIterator::frequency()
{
	m_posinfoIterator.skipDoc( m_docno);
	return m_posinfoIterator.frequency();
}

Index PostingIterator::documentFrequency() const
{
	return m_posinfoIterator.documentFrequency();
}

