/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of the iterator on statistics of the storage to populate in a registration/deregistration phase
/// \file statisticsInitIterator.hpp
#ifndef _STRUS_STATISTICS_INIT_ITERATOR_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_STATISTICS_INIT_ITERATOR_IMPLEMENTATION_HPP_INCLUDED

namespace strus
{
/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class ErrorBufferInterface;
/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class StatisticsIteratorInterface;

StatisticsIteratorInterface*
	createStatisticsInitIterator(
		const StorageClientInterface* storage,
		const DatabaseClientInterface* database,
		ErrorBufferInterface* errorhnd);

}//namespace
#endif

