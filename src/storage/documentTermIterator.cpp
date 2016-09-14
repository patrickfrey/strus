/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
	,m_docno(0)
	,m_dbadapter_inv(database_)
	,m_dbadapter_termno(database_)
	,m_invblkitr(0)
	,m_errorhnd(errorhnd_)
{
	if (!m_typeno) throw strus::runtime_error(_TXT("term type '%s' not defined in storage"), type_.c_str());
}

Index DocumentTermIterator::skipDoc( const Index& docno_)
{
	if (m_docno && m_docno == docno_) return m_docno;
	try
	{
		m_invblk.clear();
		m_invblkitr = 0;
		if (m_dbadapter_inv.loadUpperBound( docno_, m_invblk))
		{
			return m_docno = m_invblk.id();
		}
		else
		{
			return m_docno = 0;
		}
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error in document term iterator skip doc: %s"), *m_errorhnd, 0);
}

bool DocumentTermIterator::nextTerm( Term& value)
{
	try
	{
		if (!m_docno) return false;
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


