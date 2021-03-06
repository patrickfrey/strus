/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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

#define INTERFACE_NAME "posting iterator"

#undef STRUS_READABLE_FEATUREID

#ifdef STRUS_READABLE_FEATUREID
PostingIterator::PostingIterator(
		const StorageClient* storage_,
		const DatabaseClientInterface* database_,
		strus::Index termtypeno,
		strus::Index termvalueno, const char* termstr,
		strus::Index length_,
		const strus::TermStatistics& stats_,
		ErrorBufferInterface* errorhnd_)
#else
PostingIterator::PostingIterator(
		const StorageClient* storage_,
		const DatabaseClientInterface* database_,
		strus::Index termtypeno,
		strus::Index termvalueno,
		const char*,
		strus::Index length_,
		const strus::TermStatistics& stats_,
		ErrorBufferInterface* errorhnd_)
#endif
	:m_docnoIterator(database_, DatabaseKey::DocListBlockPrefix, BlockKey( termtypeno, termvalueno), true)
	,m_posinfoIterator(storage_,database_, termtypeno, termvalueno, stats_.documentFrequency())
	,m_docno(0)
	,m_termtypeno( termtypeno)
	,m_termvalueno( termvalueno)
	,m_length(length_)
	,m_errorhnd(errorhnd_)
{
	m_featureid.reserve( 16);
#ifdef STRUS_READABLE_FEATUREID
	m_featureid.append( termstr);
	m_featureid.push_back(':');
	m_featureid.push_back( (char)(termtypeno/10) + '0');
	m_featureid.push_back( (char)(termtypeno%10) + '0');
#else
	packIndex( m_featureid, termtypeno);
	packIndex( m_featureid, termvalueno);
#endif
}

Index PostingIterator::skipDoc_impl( strus::Index docno_)
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

Index PostingIterator::skipDoc( const Index& docno_)
{
	try
	{
		return skipDoc_impl( docno_);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in %s skip document: %s"), INTERFACE_NAME, *m_errorhnd, 0);
}

Index PostingIterator::skipDocCandidate( const Index& docno_)
{
	try
	{
		return skipDoc_impl( docno_);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in %s skip document candidate: %s"), INTERFACE_NAME, *m_errorhnd, 0);
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
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in %s skip skip position: %s"), INTERFACE_NAME, *m_errorhnd, 0);
}

int PostingIterator::frequency()
{
	try
	{
		if (!m_docno)
		{
			return 0;
		}
		m_posinfoIterator.skipDoc( m_docno);
		return m_posinfoIterator.frequency();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in %s get frequency: %s"), INTERFACE_NAME, *m_errorhnd, 0);
}

GlobalCounter PostingIterator::documentFrequency() const
{
	try
	{
		return m_posinfoIterator.documentFrequency();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in %s get document frequency: %s"), INTERFACE_NAME, *m_errorhnd, 0);
}

