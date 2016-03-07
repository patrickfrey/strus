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
	StatisticsProcessorInterface::BuilderOptions options( StatisticsProcessorInterface::BuilderOptions::InsertInLexicalOrder);
	m_statisticsBuilder.reset( m_proc->createBuilder( options));
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

bool StatisticsInitIterator::getNext( const char*& msg, std::size_t& msgsize)
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


