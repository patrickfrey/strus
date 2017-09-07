/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of the iterators on statistics messages of the local storage updates to populate 
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

bool StatisticsUpdateIterator::getNext( const void*& msg, std::size_t& msgsize)
{
	try
	{
		return m_storage->fetchNextStatisticsMessage( msg, msgsize);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error fetching statistics message from storage: %s"), *m_errorhnd, false);
}


