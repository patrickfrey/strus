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

InitialStatsPopulateState::InitialStatsPopulateState( const PeerMessageProcessorInterface* peermsgproc_, DatabaseClientInterface* dbclient, const Index& nofDocuments_)
{
}

bool InitialStatsPopulateState::fetchMessage( const char* blk, std::size_t blksize)
{
	return m_peerMessageBuilder->fetchMessage( blk, blksize);
}

void InitialStatsPopulateState::done()
{
	const char* blk;
	std::size_t blksize;
	if (m_peerMessageBuilder->fetchMessage( blk, blksize))
	{
		throw strus::runtime_error( _TXT( "closing peer message builder for initial messages without consuming all messages"));
	}
	m_peerMessageBuilder.reset();
	m_peermsgproc = 0;
}

