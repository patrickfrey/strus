/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of the iterator statistics messages of the storage initialization to populate
/// \file statisticsInitIterator.cpp
#include "statisticsInitIterator.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/statisticsBuilderInterface.hpp"
#include "strus/statisticsProcessorInterface.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "databaseAdapter.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <map>

using namespace strus;

StatisticsInitIterator::StatisticsInitIterator(
		StorageClientInterface* storage_,
		DatabaseClientInterface* database,
		bool sign,
		ErrorBufferInterface* errorhnd_)
	:m_storage(storage_)
	,m_proc(storage_->getStatisticsProcessor())
	,m_statisticsBuilder()
	,m_errorhnd(errorhnd_)
{
	m_statisticsBuilder.reset( m_proc->createBuilder());
	if (!m_statisticsBuilder.get())
	{
		throw strus::runtime_error(_TXT("error creating peer message builder: %s"), m_errorhnd->fetchError());
	}
	int nofdocs = m_storage->nofDocumentsInserted();
	m_statisticsBuilder->setNofDocumentsInsertedChange( sign?nofdocs:-nofdocs);
	m_statisticsBuilder->start();

	std::map<Index,std::size_t> typenomap;
	std::map<Index,std::size_t> termnomap;
	std::string strings;

	// Fill a map with the strings of all types in the collection:
	{
		DatabaseAdapter_TermType::Cursor typecursor( database);
		Index typeno;
		std::string typestr;
		for (bool more=typecursor.loadFirst( typestr, typeno); more;
			more=typecursor.loadNext( typestr, typeno))
		{
			typenomap[ typeno] = strings.size();
			strings.append( typestr);
			strings.push_back( '\0');
		}
	}
	// Fill a map with the strings of all terms in the collection:
	{
		DatabaseAdapter_TermValue::Cursor termcursor( database);
		Index termno;
		std::string termstr;
		for (bool more=termcursor.loadFirst( termstr, termno); more;
			more=termcursor.loadNext( termstr, termno))
		{
			termnomap[ termno] = strings.size();
			strings.append( termstr);
			strings.push_back( '\0');
		}
	}
	// Feed all df changes to the peer message builder:
	{
		Index typeno;
		Index termno;
		Index df;

		DatabaseAdapter_DocFrequency::Cursor dfcursor( database);
		for (bool more=dfcursor.loadFirst( typeno, termno, df); more;
			more=dfcursor.loadNext( typeno, termno, df))
		{
			std::map<Index,std::size_t>::const_iterator ti;
			ti = typenomap.find( typeno);
			if (ti == typenomap.end()) throw strus::runtime_error( _TXT( "encountered undefined type (no %d) when populating df's"), (int)typeno);
			const char* typenam = strings.c_str() + ti->second;
	
			ti = termnomap.find( termno);
			if (ti == termnomap.end()) throw strus::runtime_error( _TXT( "encountered undefined term when populating df's"));
			const char* termnam = strings.c_str() + ti->second;
	
			m_statisticsBuilder->addDfChange( typenam, termnam, sign?df:-df);
		}
	}
}

bool StatisticsInitIterator::getNext( const void*& msg, std::size_t& msgsize)
{
	if (m_errorhnd->hasError())
	{
		m_errorhnd->explain( _TXT("calling peer message queue fetch with pending error: %s"));
		return false;
	}
	try
	{
		bool rt = true;
		do
		{
			rt = m_statisticsBuilder->fetchMessage( msg, msgsize);
		}
		while (rt && msgsize == 0);
		if (rt)
		{
			return true;
		}
		else if (m_errorhnd->hasError())
		{
			m_errorhnd->explain( _TXT("error fetching initialization peer message from storage: %s"));
			return false;
		}
		else
		{
			m_statisticsBuilder.reset();
			return false;
		}
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error fetching peer message from storage: %s"), *m_errorhnd, false);
}


