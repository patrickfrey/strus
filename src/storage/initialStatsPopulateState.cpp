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
///\brief Class that manages the population of statistics to other peers
///\file initialStatsPopulateState.cpp
#include "initialStatsPopulateState.hpp"
#include "private/internationalization.hpp"
#include "strus/databaseClientInterface.hpp"
#include "databaseAdapter.hpp"
#include <vector>
#include <string>
#include <map>

using namespace strus;

void InitialStatsPopulateState::init( const PeerMessageProcessorInterface* peermsgproc_, DatabaseClientInterface* dbclient, const Index& nofDocuments_)
{
	m_peermsgproc = peermsgproc_;
	PeerMessageProcessorInterface::BuilderOptions options( PeerMessageProcessorInterface::BuilderOptions::InsertInLexicalOrder);
	m_peerMessageBuilder.reset( m_peermsgproc->createBuilder( options, &m_errorhnd));
	if (!m_peerMessageBuilder.get()) throw strus::runtime_error(_TXT("error creating peer message builder: %s"), m_errorhnd.hasError()?m_errorhnd.fetchError():"unknown error");
	m_peerMessageBuilder->setNofDocumentsInsertedChange( nofDocuments_);
	m_peerMessageBuilder->start();

	std::map<Index,std::size_t> typenomap;
	std::map<Index,std::size_t> termnomap;
	std::string strings;

	// Fill a map with the strings of all types in the collection:
	{
		DatabaseAdapter_TermType::Cursor typecursor( dbclient);
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
		DatabaseAdapter_TermValue::Cursor termcursor( dbclient);
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

		DatabaseAdapter_DocFrequency::Cursor dfcursor( dbclient);
		for (bool more=dfcursor.loadFirst( typeno, termno, df); more;
			more=dfcursor.loadNext( typeno, termno, df))
		{
			std::map<Index,std::size_t>::const_iterator ti;
			ti = typenomap.find( typeno);
			if (ti == typenomap.end()) throw strus::runtime_error( _TXT( "encountered undefined type when populating df's"));
			const char* typenam = strings.c_str() + ti->second;
	
			ti = termnomap.find( termno);
			if (ti == termnomap.end()) throw strus::runtime_error( _TXT( "encountered undefined term when populating df's"));
			const char* termnam = strings.c_str() + ti->second;
	
			m_peerMessageBuilder->addDfChange( typenam, termnam, df, true/*isNew*/);
		}
	}
}

bool InitialStatsPopulateState::fetchMessage( const char* blk, std::size_t blksize)
{
	if (!m_peermsgproc) return false;

	if (m_peerMessageBuilder->fetchMessage( blk, blksize))
	{
		return true;
	}
	else
	{
		const char* errmsg = m_errorhnd.fetchError();
		m_peermsgproc = 0;
		if (errmsg)
		{
			throw strus::runtime_error( _TXT("error fetching initial statistics for other peers: %s"), errmsg);
		}
		return false;
	}
}

