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
/// \brief Implementation of the iterators on peer messages of the local storage updates for other peers
/// \file statisticsUpdateIterator.cpp
#include "statisticsUpdateIterator.hpp"
#include "strus/statisticsBuilderInterface.hpp"
#include "strus/statisticsProcessorInterface.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "storageClient.hpp"

using namespace strus;

StatisticsUpdateIterator::StatisticsUpdateIterator(
		StorageClient* storage_,
		ErrorBufferInterface* errorhnd_)
	:m_storage(storage_)
	,m_errorhnd(errorhnd_)
{}

bool StatisticsUpdateIterator::getNext( const char*& msg, std::size_t& msgsize)
{
	try
	{
		return m_storage->fetchPeerUpdateMessage( msg, msgsize);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error fetching peer message from storage: %s"), *m_errorhnd, false);
}


