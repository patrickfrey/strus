/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
/// \brief Implementation of the iterator on term occurrencies in documents (intended for feature selection)
/// \file "DocumentTermIterator.cpp"
#include "documentTermIterator.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "storageClient.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"

using namespace strus;

DocumentTermIterator::DocumentTermIterator(
		const StorageClient* storage_,
		const DatabaseClientInterface* database_,
		const std::string& type_,
		ErrorBufferInterface* errorhnd_)
	:m_storage(storage_)
	,m_database(database_)
	,m_typeno(storage_->getTermType(type_))
	,m_dbadapter_inv(database_)
	,m_dbadapter_termno(database_)
	,m_invblkitr(0)
	,m_errorhnd(errorhnd_){}

bool DocumentTermIterator::skipDoc( const Index& docno)
{
	try
	{
		m_invblk.clear();
		m_invblkitr = 0;
		if (m_typeno) return m_dbadapter_inv.load( docno, m_invblk);
	}
	CATCH_ERROR_MAP( _TXT("error in document term iterator skip doc: %s"), *m_errorhnd);
}

bool DocumentTermIterator::nextTerm( Term& value)
{
	try
	{
		if (!m_invblkitr)
		{
			m_invblkitr = m_invblk.begin();
		}
		else
		{
			m_invblkitr = m_invblk.next( m_invblkitr);
		}
		for (; m_invblkitr != m_invblk.end(); m_invblkitr=m_invblk.next( m_invblkitr))
		{
			InvTermBlock::Element elem = m_invblk.element_at( m_invblkitr);
			if (elem.typeno == m_typeno)
			{
				value.init( elem.ff, elem.firstpos, elem.termno);
				return true;
			}
		}
		return false;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error in document term iterator next term: %s"), *m_errorhnd, false);
}

unsigned int DocumentTermIterator::termDocumentFrequency( const Index& termno) const
{
	try
	{
		return m_storage->documentFrequency( m_typeno, termno);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error in document term iterator get document frequency: %s"), *m_errorhnd, 0);
}

std::string DocumentTermIterator::termValue( const Index& termno) const
{
	try
	{
		std::string rt;
		m_dbadapter_termno.load( termno, rt);
		return rt;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error in document term iterator get document frequency: %s"), *m_errorhnd, std::string());
}


