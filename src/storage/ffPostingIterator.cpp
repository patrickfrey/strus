/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ffPostingIterator.hpp"
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

#define INTERFACE_NAME "frequency posting iterator"

#undef STRUS_READABLE_FEATUREID

#ifdef STRUS_READABLE_FEATUREID
FfPostingIterator::FfPostingIterator(
		const StorageClient* storage_,
		const DatabaseClientInterface* database_,
		const Index& termtypeno,
		const Index& termvalueno, const char* termstr,
		ErrorBufferInterface* errorhnd_)
#else
FfPostingIterator::FfPostingIterator(
		const StorageClient* storage_,
		const DatabaseClientInterface* database_,
		const Index& termtypeno,
		const Index& termvalueno,
		const char*,
		ErrorBufferInterface* errorhnd_)
#endif
	:m_docnoIterator(database_, DatabaseKey::DocListBlockPrefix, BlockKey( termtypeno, termvalueno), true)
	,m_ffIterator(storage_,database_, termtypeno, termvalueno)
	,m_docno(0)
	,m_termtypeno( termtypeno)
	,m_termvalueno( termvalueno)
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

Index FfPostingIterator::skipDoc_impl( const Index& docno_)
{
	if (m_docno && m_docno == docno_) return m_docno;

	if (m_ffIterator.isCloseCandidate( docno_))
	{
		m_docno = m_ffIterator.skipDoc( docno_);
	}
	else
	{
		m_docno = m_docnoIterator.skip( docno_);
	}
	return m_docno;
}

Index FfPostingIterator::skipDoc( const Index& docno_)
{
	try
	{
		return skipDoc_impl( docno_);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in %s skip document: %s"), INTERFACE_NAME, *m_errorhnd, 0);
}

Index FfPostingIterator::skipDocCandidate( const Index& docno_)
{
	try
	{
		return skipDoc_impl( docno_);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in %s skip document candidate: %s"), INTERFACE_NAME, *m_errorhnd, 0);
}

Index FfPostingIterator::skipPos( const Index& firstpos_)
{
	try
	{
		if (!m_docno)
		{
			return 0;
		}
		if (m_docno != m_ffIterator.skipDoc( m_docno))
		{
			return 0;
		}
		return m_ffIterator.skipPos( firstpos_);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in %s skip skip position: %s"), INTERFACE_NAME, *m_errorhnd, 0);
}

int FfPostingIterator::frequency()
{
	try
	{
		if (!m_docno)
		{
			return 0;
		}
		m_ffIterator.skipDoc( m_docno);
		return m_ffIterator.frequency();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in %s get frequency: %s"), INTERFACE_NAME, *m_errorhnd, 0);
}

Index FfPostingIterator::documentFrequency() const
{
	try
	{
		return m_ffIterator.documentFrequency();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in %s get document frequency: %s"), INTERFACE_NAME, *m_errorhnd, 0);
}




#ifdef STRUS_READABLE_FEATUREID
FfNoIndexSetPostingIterator::FfNoIndexSetPostingIterator(
		const StorageClient* storage_,
		const DatabaseClientInterface* database_,
		const Index& termtypeno,
		const Index& termvalueno, const char* termstr,
		ErrorBufferInterface* errorhnd_)
#else
FfNoIndexSetPostingIterator::FfNoIndexSetPostingIterator(
		const StorageClient* storage_,
		const DatabaseClientInterface* database_,
		const Index& termtypeno,
		const Index& termvalueno,
		const char*,
		ErrorBufferInterface* errorhnd_)
#endif
	:m_ffIterator(storage_,database_, termtypeno, termvalueno)
	,m_docno(0)
	,m_termtypeno( termtypeno)
	,m_termvalueno( termvalueno)
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

Index FfNoIndexSetPostingIterator::skipDoc_impl( const Index& docno_)
{
	if (m_docno && m_docno == docno_) return m_docno;
	return m_docno = m_ffIterator.skipDoc( docno_);
}

Index FfNoIndexSetPostingIterator::skipDoc( const Index& docno_)
{
	try
	{
		return skipDoc_impl( docno_);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in %s skip document: %s"), INTERFACE_NAME, *m_errorhnd, 0);
}

Index FfNoIndexSetPostingIterator::skipDocCandidate( const Index& docno_)
{
	try
	{
		return skipDoc_impl( docno_);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in %s skip document candidate: %s"), INTERFACE_NAME, *m_errorhnd, 0);
}

Index FfNoIndexSetPostingIterator::skipPos( const Index& firstpos_)
{
	try
	{
		if (!m_docno) return 0;
		return m_ffIterator.skipPos( firstpos_);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in %s skip skip position: %s"), INTERFACE_NAME, *m_errorhnd, 0);
}

int FfNoIndexSetPostingIterator::frequency()
{
	try
	{
		return m_ffIterator.frequency();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in %s get frequency: %s"), INTERFACE_NAME, *m_errorhnd, 0);
}

Index FfNoIndexSetPostingIterator::documentFrequency() const
{
	try
	{
		return m_ffIterator.documentFrequency();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in %s get document frequency: %s"), INTERFACE_NAME, *m_errorhnd, 0);
}

