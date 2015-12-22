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
#include "storageClient.hpp"
#include "indexPacker.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <string>
#include <vector>
#include <cstring>
#include <sstream>
#include <iostream>

using namespace strus;

#undef STRUS_LOWLEVEL_DEBUG

#ifdef STRUS_LOWLEVEL_DEBUG
PostingIterator::PostingIterator(
		const StorageClient* storage_,
		const DatabaseClientInterface* database_,
		const Index& termtypeno,
		const Index& termvalueno, const char* termstr,
		ErrorBufferInterface* errorhnd_)
#else
PostingIterator::PostingIterator(
		const StorageClient* storage_,
		const DatabaseClientInterface* database_,
		const Index& termtypeno,
		const Index& termvalueno,
		const char*,
		ErrorBufferInterface* errorhnd_)
#endif
	:m_docnoIterator(database_, DatabaseKey::DocListBlockPrefix, BlockKey( termtypeno, termvalueno), true)
	,m_posinfoIterator(storage_,database_, termtypeno, termvalueno)
	,m_docno(0)
	,m_errorhnd(errorhnd_)
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
	try
	{
		if (m_docno && m_docno == docno_) return m_docno;
	
		if (m_posinfoIterator.isCloseCandidate( docno_))
		{
			m_docno = m_posinfoIterator.skipDoc( docno_);
		}
		else
		{
			m_docno = m_docnoIterator.skip( docno_);
		}
		return m_docno;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error in posting iterator skip document: %s"), *m_errorhnd, 0);
}

Index PostingIterator::skipPos( const Index& firstpos_)
{
	try
	{
		if (!m_docno)
		{
			return 0;
		}
		if (m_docno != m_posinfoIterator.skipDoc( m_docno))
		{
			return 0;
		}
		return m_posinfoIterator.skipPos( firstpos_);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error in posting iterator skip position: %s"), *m_errorhnd, 0);
}

unsigned int PostingIterator::frequency()
{
	try
	{
		m_posinfoIterator.skipDoc( m_docno);
		return m_posinfoIterator.frequency();
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error in posting iterator get frequency: %s"), *m_errorhnd, 0);
}

GlobalCounter PostingIterator::documentFrequency() const
{
	try
	{
		return m_posinfoIterator.documentFrequency();
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error in posting iterator get document frequency: %s"), *m_errorhnd, 0);
}

