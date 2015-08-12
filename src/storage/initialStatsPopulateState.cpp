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
	PeerMessageProcessorInterface::BuilderFlags flags( PeerMessageProcessorInterface::BuilderFlags::InsertInLexicalOrder);
	m_peerMessageBuilder.reset( m_peermsgproc->createBuilder( flags));
	m_peerMessageBuilder->setNofDocumentsInsertedChange( nofDocuments_);
	m_peerMessageBuilder->start();
}

void InitialStatsPopulateState::clear()
{
	m_peerMessageBuilder->rollback();
	m_peerMessageBuilder->start();
	m_typenomap.clear();
	m_termnomap.clear();
	m_strings.clear();
}

void InitialStatsPopulateState::addTypeDef( const std::string& name, const Index& no)
{
	m_typenomap[ no] = m_strings.size();
	m_strings.append( name);
	m_strings.push_back( '\0');
}

void InitialStatsPopulateState::addTermDef( const std::string& name, const Index& no)
{
	m_termnomap[ no] = m_strings.size();
	m_strings.append( name);
	m_strings.push_back( '\0');
}

const char* InitialStatsPopulateState::getTypeName( const Index& no) const
{
	std::map<Index,std::size_t>::const_iterator ti = m_typenomap.find(no);
	if (ti == m_typenomap.end()) throw strus::runtime_error( _TXT( "encountered undefined type when populating df's"));
	return m_strings.c_str() + ti->second;
}

const char* InitialStatsPopulateState::getTermName( const Index& no) const
{
	std::map<Index,std::size_t>::const_iterator ti = m_termnomap.find(no);
	if (ti == m_termnomap.end()) throw strus::runtime_error( _TXT( "encountered undefined term when populating df's"));
	return m_strings.c_str() + ti->second;
}

std::string InitialStatsPopulateState::fetch()
{
	// NOTE: This implementation does fetch the whole df map in one step.
	//	Better implementations might do this differently.
	if (!m_peermsgproc) return std::string();

	try
	{
		Index typeno;
		Index termno;
		Index df;
	
		m_peerMessageBuilder->rollback();
		m_peerMessageBuilder->start();
		// Fill a map with the strings of all types in the collection:
		{
			DatabaseAdapter_TermType::Cursor typecursor( m_database);
			Index typeno;
			std::string typestr;
			for (bool more=typecursor.loadFirst( typestr, typeno); more;
				more=typecursor.loadNext( typestr, typeno))
			{
				addTypeDef( typestr, typeno);
			}
		}
		// Fill a map with the strings of all terms in the collection:
		{
			DatabaseAdapter_TermValue::Cursor termcursor( m_database);
			Index termno;
			std::string termstr;
			for (bool more=termcursor.loadFirst( termstr, termno); more;
				more=termcursor.loadNext( termstr, termno))
			{
				addTermDef( termstr, termno);
			}
		}
		DatabaseAdapter_DocFrequency::Cursor dfcursor( m_database);
		for (bool more=dfcursor.loadFirst( typeno, termno, df); more;
			more=dfcursor.loadNext( typeno, termno, df))
		{
			m_peerMessageBuilder->addDfChange(
				getTypeName(typeno), getTermName(termno), df, true/*isNew*/);
		}
		std::string rt = m_peerMessageBuilder->fetch();
		clear();
		m_peermsgproc = 0;
		return rt;
	}
	catch (const std::bad_alloc& err)
	{
		clear();
		throw err;
	}
	catch (const std::runtime_error& err)
	{
		clear();
		throw err;
	}
}

